#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using boost::multiprecision::cpp_int;

static constexpr int C = 5;
static constexpr int N = 2 * C;
static constexpr uint64_t GROUP_ORDER = (1u << C) * 120u; // 2^5 * 5!

using VertexMap = array<uint8_t, N>;

struct Config {
    array<uint16_t, N> a{};
    bool operator==(Config const& o) const noexcept { return a == o.a; }
    bool operator<(Config const& o) const noexcept { return a < o.a; }
};

struct ConfigHash {
    size_t operator()(Config const& x) const noexcept {
        uint64_t h = 0x9e3779b97f4a7c15ULL;
        for (uint16_t v : x.a) {
            h ^= uint64_t(v) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return size_t(h);
    }
};

static inline uint64_t mix64(uint64_t x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static string u128str(unsigned __int128 x) {
    if (!x) return "0";
    string s;
    while (x) {
        s.push_back(char('0' + unsigned(x % 10)));
        x /= 10;
    }
    reverse(s.begin(), s.end());
    return s;
}

static unsigned __int128 parse_u128(const string& s) {
    unsigned __int128 x = 0;
    for (char ch : s) {
        if (ch >= '0' && ch <= '9') x = x * 10 + unsigned(ch - '0');
    }
    return x;
}

// Compact exact open-addressing map from a 40-bit raw U key to a 16-bit response id.
// One 64-bit word per slot. Empty is zero. Low 40 bits store key+1; high bits id+1.
class RawResponseTable {
public:
    static constexpr unsigned KEY_BITS = 40;
    static constexpr uint64_t KEY_MASK = (uint64_t(1) << KEY_BITS) - 1;

    explicit RawResponseTable(size_t log2_capacity)
        : slots_(size_t(1) << log2_capacity, 0), mask_(slots_.size() - 1) {}

    bool get(uint64_t key, uint16_t& id, uint64_t* probes = nullptr) const noexcept {
        const uint64_t kp = key + 1;
        size_t pos = size_t(mix64(key)) & mask_;
        uint64_t p = 0;
        for (;;) {
            ++p;
            uint64_t w = slots_[pos];
            if (w == 0) {
                if (probes) *probes += p;
                return false;
            }
            if ((w & KEY_MASK) == kp) {
                id = uint16_t((w >> KEY_BITS) - 1);
                if (probes) *probes += p;
                return true;
            }
            pos = (pos + 1) & mask_;
        }
    }

    void put(uint64_t key, uint16_t id, uint64_t* probes = nullptr) {
        const uint64_t kp = key + 1;
        const uint64_t word = (uint64_t(id + 1) << KEY_BITS) | kp;
        size_t pos = size_t(mix64(key)) & mask_;
        uint64_t p = 0;
        for (;;) {
            ++p;
            uint64_t& w = slots_[pos];
            if (w == 0) {
                w = word;
                ++size_;
                if (probes) *probes += p;
                return;
            }
            if ((w & KEY_MASK) == kp) {
                uint16_t old = uint16_t((w >> KEY_BITS) - 1);
                if (old != id) {
                    cerr << "response-table inconsistency: key " << key
                         << " old=" << old << " new=" << id << "\n";
                    exit(3);
                }
                if (probes) *probes += p;
                return;
            }
            pos = (pos + 1) & mask_;
        }
    }

    size_t size() const noexcept { return size_; }
    size_t capacity() const noexcept { return slots_.size(); }
    size_t bytes() const noexcept { return slots_.size() * sizeof(uint64_t); }

private:
    vector<uint64_t> slots_;
    size_t mask_;
    size_t size_ = 0;
};

vector<VertexMap> G;
array<array<uint8_t, C>, GROUP_ORDER> pair_perm{};
array<array<uint8_t, C>, GROUP_ORDER> pair_flip{};

static Config transform_config(const Config& x, const VertexMap& m) {
    Config z;
    for (int k = 0; k < N; ++k) {
        uint16_t v = x.a[k], out = 0;
        while (v) {
            int b = __builtin_ctz(unsigned(v));
            v &= uint16_t(v - 1);
            out |= uint16_t(1u << m[b]);
        }
        z.a[k] = out;
    }
    sort(z.a.begin(), z.a.end());
    return z;
}

static void generate_group() {
    array<int, C> p{};
    iota(p.begin(), p.end(), 0);
    size_t gi = 0;
    do {
        for (int f = 0; f < (1 << C); ++f) {
            VertexMap m{};
            for (int i = 0; i < C; ++i) {
                pair_perm[gi][i] = uint8_t(p[i]);
                pair_flip[gi][i] = uint8_t((f >> i) & 1);
                for (int side = 0; side < 2; ++side) {
                    m[2 * i + side] = uint8_t(2 * p[i] + (side ^ ((f >> i) & 1)));
                }
            }
            G.push_back(m);
            ++gi;
        }
    } while (next_permutation(p.begin(), p.end()));
    if (G.size() != GROUP_ORDER) {
        cerr << "bad group size\n";
        exit(2);
    }
}

// Pair-code helpers. A local missing-coordinate group is an unordered pair
// 0 <= a <= b < 16, encoded as b(b+1)/2 + a in [0,135].
array<uint8_t, 136> pair_a{};
array<uint8_t, 136> pair_b{};
array<array<array<uint8_t, 136>, C>, GROUP_ORDER> transformed_pair{};

static uint8_t encode_pair(int a, int b) {
    if (a > b) swap(a, b);
    return uint8_t(b * (b + 1) / 2 + a);
}

static void initialize_pair_codes() {
    for (int b = 0; b < 16; ++b) {
        for (int a = 0; a <= b; ++a) {
            uint8_t id = encode_pair(a, b);
            pair_a[id] = uint8_t(a);
            pair_b[id] = uint8_t(b);
        }
    }

    for (size_t gi = 0; gi < G.size(); ++gi) {
        for (int missing = 0; missing < C; ++missing) {
            int target_missing = pair_perm[gi][missing];
            (void)target_missing;
            for (int id = 0; id < 136; ++id) {
                auto transform_pattern = [&](int pat) {
                    int out = 0;
                    int src_bit = 0;
                    for (int q = 0; q < C; ++q) {
                        if (q == missing) continue;
                        int side = ((pat >> src_bit) & 1) ^ pair_flip[gi][q];
                        int tq = pair_perm[gi][q];
                        int dst_bit = tq - (tq > target_missing ? 1 : 0);
                        out |= side << dst_bit;
                        ++src_bit;
                    }
                    return out;
                };
                int a = transform_pattern(pair_a[id]);
                int b = transform_pattern(pair_b[id]);
                transformed_pair[gi][missing][id] = encode_pair(a, b);
            }
        }
    }
}

static uint64_t transform_u_key(uint64_t key, size_t gi) noexcept {
    uint64_t out = 0;
    for (int p = 0; p < C; ++p) {
        uint8_t id = uint8_t((key >> (8 * p)) & 0xff);
        int tp = pair_perm[gi][p];
        out |= uint64_t(transformed_pair[gi][p][id]) << (8 * tp);
    }
    return out;
}

static uint64_t pack_q(const array<uint8_t, N>& words) {
    uint64_t key = 0;
    for (int i = 0; i < N; ++i) key |= uint64_t(words[i]) << (C * i);
    return key;
}

static array<uint8_t, N> unpack_q(uint64_t key) {
    array<uint8_t, N> words{};
    for (int i = 0; i < N; ++i) words[i] = uint8_t((key >> (C * i)) & ((1u << C) - 1));
    return words;
}

static uint8_t transform_word(uint8_t w, size_t gi) {
    uint8_t out = 0;
    for (int p = 0; p < C; ++p) {
        int side = ((w >> p) & 1) ^ pair_flip[gi][p];
        out |= uint8_t(side << pair_perm[gi][p]);
    }
    return out;
}

static uint64_t transform_q_key(uint64_t key, size_t gi) {
    auto words = unpack_q(key);
    for (uint8_t& w : words) w = transform_word(w, gi);
    sort(words.begin(), words.end());
    return pack_q(words);
}

static void generate_balanced_q_rec(
    int word,
    int remaining,
    array<int, C>& ones,
    array<uint8_t, N>& out,
    int pos,
    vector<uint64_t>& keys) {
    if (word == (1 << C)) {
        if (remaining == 0) {
            for (int p = 0; p < C; ++p) if (ones[p] != C) return;
            keys.push_back(pack_q(out));
        }
        return;
    }
    if (remaining == 0) {
        for (int p = 0; p < C; ++p) if (ones[p] != C) return;
        keys.push_back(pack_q(out));
        return;
    }

    for (int count = 0; count <= remaining; ++count) {
        bool feasible = true;
        for (int p = 0; p < C; ++p) {
            int now = ones[p] + count * ((word >> p) & 1);
            if (now > C || now + (remaining - count) < C) {
                feasible = false;
                break;
            }
        }
        if (!feasible) continue;
        for (int t = 0; t < count; ++t) out[pos + t] = uint8_t(word);
        for (int p = 0; p < C; ++p) ones[p] += count * ((word >> p) & 1);
        generate_balanced_q_rec(word + 1, remaining - count, ones, out, pos + count, keys);
        for (int p = 0; p < C; ++p) ones[p] -= count * ((word >> p) & 1);
    }
}

struct U64Hash {
    size_t operator()(uint64_t x) const noexcept { return size_t(mix64(x)); }
};

struct QOrbit {
    uint64_t representative = 0;
    uint16_t stabilizer = 0;
    uint16_t orbit_size = 0;
    uint32_t labelled_multiplicity = 0;
};

unordered_map<uint64_t, uint16_t, U64Hash> q_id;
vector<QOrbit> q_orbits;

static void build_complete_orbit_index() {
    vector<uint64_t> qkeys;
    qkeys.reserve(589392);
    array<int, C> ones{};
    array<uint8_t, N> out{};
    generate_balanced_q_rec(0, N, ones, out, 0, qkeys);
    sort(qkeys.begin(), qkeys.end());
    qkeys.erase(unique(qkeys.begin(), qkeys.end()), qkeys.end());
    if (qkeys.size() != 589392) {
        cerr << "unexpected concrete complete count " << qkeys.size() << "\n";
        exit(2);
    }

    unordered_set<uint64_t, U64Hash> unseen;
    unseen.reserve(qkeys.size() * 2);
    unseen.insert(qkeys.begin(), qkeys.end());
    q_id.reserve(qkeys.size() * 2);

    while (!unseen.empty()) {
        uint64_t seed = *unseen.begin();
        vector<uint64_t> images;
        images.reserve(G.size());
        for (size_t gi = 0; gi < G.size(); ++gi) images.push_back(transform_q_key(seed, gi));
        sort(images.begin(), images.end());
        images.erase(unique(images.begin(), images.end()), images.end());

        uint16_t id = uint16_t(q_orbits.size());
        for (uint64_t z : images) {
            unseen.erase(z);
            q_id.emplace(z, id);
        }

        QOrbit o;
        o.representative = images.front();
        o.orbit_size = uint16_t(images.size());
        o.stabilizer = uint16_t(G.size() / images.size());
        auto words = unpack_q(o.representative);
        uint64_t ell = 1;
        for (int k = 2; k <= N; ++k) ell *= k;
        for (int i = 0, j; i < N; i = j) {
            j = i + 1;
            while (j < N && words[j] == words[i]) ++j;
            for (int k = 2; k <= j - i; ++k) ell /= k;
        }
        o.labelled_multiplicity = uint32_t(ell);
        q_orbits.push_back(o);
    }

    if (q_orbits.size() != 355 || q_id.size() != qkeys.size()) {
        cerr << "unexpected complete orbit inventory: orbits=" << q_orbits.size()
             << " map=" << q_id.size() << "\n";
        exit(2);
    }

    // Release the large unseen bucket array before allocating the raw quotient table.
    unordered_set<uint64_t, U64Hash>().swap(unseen);
}

struct ResponseSignature {
    array<uint16_t, 32> q{};
    array<uint32_t, 32> coefficient{};
    uint8_t size = 0;
    bool operator==(ResponseSignature const& o) const noexcept {
        if (size != o.size) return false;
        for (int i = 0; i < size; ++i) {
            if (q[i] != o.q[i] || coefficient[i] != o.coefficient[i]) return false;
        }
        return true;
    }
};

struct ResponseHash {
    size_t operator()(ResponseSignature const& s) const noexcept {
        uint64_t h = 0x9e3779b97f4a7c15ULL ^ s.size;
        for (int i = 0; i < s.size; ++i) {
            uint64_t x = (uint64_t(s.q[i]) << 32) | s.coefficient[i];
            h ^= mix64(x + h);
        }
        return size_t(h);
    }
};

static ResponseSignature compute_response_signature(uint64_t ukey, uint64_t& completion_counter) {
    array<uint8_t, C> a{}, b{};
    for (int p = 0; p < C; ++p) {
        uint8_t id = uint8_t((ukey >> (8 * p)) & 0xff);
        a[p] = pair_a[id];
        b[p] = pair_b[id];
    }

    vector<pair<uint16_t, uint32_t>> terms;
    terms.reserve(1 << C);
    array<uint8_t, N> words{};

    function<void(int)> recurse = [&](int p) {
        if (p == C) {
            ++completion_counter;
            array<uint8_t, N> sorted = words;
            sort(sorted.begin(), sorted.end());
            uint32_t kc = 1;
            for (int i = 0, j; i < N; i = j) {
                j = i + 1;
                while (j < N && sorted[j] == sorted[i]) ++j;
                for (int k = 2; k <= j - i; ++k) kc *= uint32_t(k);
            }
            uint64_t qkey = pack_q(sorted);
            auto it = q_id.find(qkey);
            if (it == q_id.end()) {
                cerr << "complete output missing from q index\n";
                exit(3);
            }
            terms.emplace_back(it->second, kc);
            return;
        }

        auto expand = [&](uint8_t pat, int missing, int missing_bit) {
            uint8_t w = uint8_t(missing_bit << missing);
            int src = 0;
            for (int q = 0; q < C; ++q) {
                if (q == missing) continue;
                w |= uint8_t(((pat >> src) & 1) << q);
                ++src;
            }
            return w;
        };

        words[2 * p] = expand(a[p], p, 0);
        words[2 * p + 1] = expand(b[p], p, 1);
        recurse(p + 1);
        if (a[p] != b[p]) {
            words[2 * p] = expand(a[p], p, 1);
            words[2 * p + 1] = expand(b[p], p, 0);
            recurse(p + 1);
        }
    };
    recurse(0);

    sort(terms.begin(), terms.end());
    ResponseSignature s;
    for (size_t i = 0; i < terms.size();) {
        size_t j = i + 1;
        uint32_t sum = terms[i].second;
        while (j < terms.size() && terms[j].first == terms[i].first) {
            sum += terms[j].second;
            ++j;
        }
        s.q[s.size] = terms[i].first;
        s.coefficient[s.size] = sum;
        ++s.size;
        i = j;
    }
    return s;
}

struct Orbit2 {
    Config representative;
    vector<uint16_t> stabilizer_elements;
    uint16_t stabilizer = 0;
    uint8_t f2 = 0;
};

static unordered_set<Config, ConfigHash> generate_two_row_concrete() {
    unordered_set<Config, ConfigHash> states;
    states.reserve(200000);
    array<int, N> p{};
    iota(p.begin(), p.end(), 0);
    do {
        bool ok = true;
        for (int i = 0; i < N; ++i) {
            if (p[i] / 2 == i / 2) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;
        Config x;
        for (int i = 0; i < N; ++i) x.a[i] = uint16_t((1u << i) | (1u << p[i]));
        sort(x.a.begin(), x.a.end());
        states.insert(x);
    } while (next_permutation(p.begin(), p.end()));
    return states;
}

static int component_count(const Config& x) {
    array<vector<int>, N> adj;
    for (uint16_t mask : x.a) {
        int a = __builtin_ctz(unsigned(mask));
        int b = __builtin_ctz(unsigned(mask & uint16_t(mask - 1)));
        adj[a].push_back(b);
        adj[b].push_back(a);
    }
    array<uint8_t, N> seen{};
    int components = 0;
    for (int start = 0; start < N; ++start) {
        if (seen[start]) continue;
        ++components;
        vector<int> stack{start};
        seen[start] = 1;
        while (!stack.empty()) {
            int u = stack.back();
            stack.pop_back();
            for (int v : adj[u]) if (!seen[v]) {
                seen[v] = 1;
                stack.push_back(v);
            }
        }
    }
    return components;
}

static vector<Orbit2> generate_two_row_orbits() {
    auto unseen = generate_two_row_concrete();
    if (unseen.size() != 165744) {
        cerr << "unexpected concrete two-row count " << unseen.size() << "\n";
        exit(2);
    }
    vector<Orbit2> orbits;
    while (!unseen.empty()) {
        Config seed = *unseen.begin();
        Config rep = seed;
        for (const auto& g : G) {
            Config z = transform_config(seed, g);
            if (z < rep) rep = z;
        }

        vector<Config> images;
        images.reserve(G.size());
        for (const auto& g : G) images.push_back(transform_config(rep, g));
        sort(images.begin(), images.end());
        images.erase(unique(images.begin(), images.end()), images.end());
        for (const Config& z : images) unseen.erase(z);

        Orbit2 o;
        o.representative = rep;
        for (size_t gi = 0; gi < G.size(); ++gi) {
            if (transform_config(rep, G[gi]) == rep) o.stabilizer_elements.push_back(uint16_t(gi));
        }
        o.stabilizer = uint16_t(o.stabilizer_elements.size());
        o.f2 = uint8_t(1u << component_count(rep));
        orbits.push_back(move(o));
    }
    sort(orbits.begin(), orbits.end(), [](const Orbit2& a, const Orbit2& b) {
        return a.representative < b.representative;
    });
    if (orbits.size() != 107) {
        cerr << "unexpected two-row orbit count " << orbits.size() << "\n";
        exit(2);
    }
    unordered_set<Config, ConfigHash>().swap(unseen);
    return orbits;
}

static inline uint8_t box_support(uint16_t mask) {
    uint8_t out = 0;
    for (int p = 0; p < C; ++p) if (mask & (3u << (2 * p))) out |= uint8_t(1u << p);
    return out;
}

struct Types {
    int count = 0;
    array<uint16_t, N> mask{};
    array<uint8_t, N> multiplicity{};
    array<uint8_t, N> boxes{};
};

static Types make_types(const Config& x) {
    Types t;
    for (int i = 0; i < N;) {
        int j = i + 1;
        while (j < N && x.a[j] == x.a[i]) ++j;
        t.mask[t.count] = x.a[i];
        t.multiplicity[t.count] = uint8_t(j - i);
        t.boxes[t.count] = box_support(x.a[i]);
        ++t.count;
        i = j;
    }
    return t;
}

array<uint8_t, 1 << N> union_missing{};
array<uint8_t, 1 << N> union_pattern{};

static void initialize_union_meta() {
    union_missing.fill(255);
    for (int mask = 0; mask < (1 << N); ++mask) {
        int missing = -1;
        int pattern = 0;
        int bit = 0;
        bool good = true;
        for (int p = 0; p < C; ++p) {
            int pairbits = (mask >> (2 * p)) & 3;
            if (pairbits == 0) {
                if (missing != -1) good = false;
                missing = p;
            } else if (pairbits == 1 || pairbits == 2) {
                // pattern is filled after missing is known below
            } else {
                good = false;
            }
        }
        if (!good || missing < 0) continue;
        for (int p = 0; p < C; ++p) {
            if (p == missing) continue;
            int pairbits = (mask >> (2 * p)) & 3;
            int side = pairbits == 2;
            pattern |= side << bit;
            ++bit;
        }
        union_missing[mask] = uint8_t(missing);
        union_pattern[mask] = uint8_t(pattern);
    }
}

struct Counters {
    uint64_t placements = 0;
    uint64_t leaves = 0;
    uint64_t raw_lookup_probes = 0;
    uint64_t raw_insert_probes = 0;
    uint64_t quotient_misses = 0;
    uint64_t orbit_image_insert_attempts = 0;
    uint64_t response_terms = 0;
    uint64_t signature_completion_records = 0;
};

class QuotientBuilder {
public:
    explicit QuotientBuilder(size_t table_log2)
        : raw_to_response(table_log2) {
        signature_to_id.reserve(25000);
        responses.reserve(18000);
        weights.reserve(18000);
        orbit_counts.reserve(18000);
    }

    uint16_t get_or_create(uint64_t ukey, Counters& counters) {
        uint16_t id;
        if (raw_to_response.get(ukey, id, &counters.raw_lookup_probes)) return id;

        ++counters.quotient_misses;
        ResponseSignature sig = compute_response_signature(ukey, counters.signature_completion_records);
        auto it = signature_to_id.find(sig);
        if (it == signature_to_id.end()) {
            if (responses.size() >= numeric_limits<uint16_t>::max()) {
                cerr << "too many response classes\n";
                exit(3);
            }
            id = uint16_t(responses.size());
            signature_to_id.emplace(sig, id);
            responses.push_back(sig);
            weights.push_back(0);
            orbit_counts.push_back(0);
        } else {
            id = it->second;
        }
        ++orbit_counts[id];

        // Exact orbit expansion: one miss identifies a previously unseen coordinate orbit,
        // because all images of every prior miss were inserted at creation time.
        for (size_t gi = 0; gi < G.size(); ++gi) {
            uint64_t image = transform_u_key(ukey, gi);
            raw_to_response.put(image, id, &counters.raw_insert_probes);
            ++counters.orbit_image_insert_attempts;
        }
        return id;
    }

    RawResponseTable raw_to_response;
    unordered_map<ResponseSignature, uint16_t, ResponseHash> signature_to_id;
    vector<ResponseSignature> responses;
    vector<unsigned __int128> weights;
    vector<uint16_t> orbit_counts;
};

class ContingencyEnumerator {
public:
    ContingencyEnumerator(
        const Config& x,
        const Config& y,
        uint64_t scalar_weight,
        QuotientBuilder& quotient,
        Counters& counters)
        : X(make_types(x)), Y(make_types(y)), scalar_weight_(scalar_weight),
          quotient_(quotient), counters_(counters) {
        for (int i = 0; i < X.count; ++i) rx[i] = X.multiplicity[i];
        for (int j = 0; j < Y.count; ++j) ry[j] = Y.multiplicity[j];
        for (int i = 0; i < X.count; ++i) {
            for (int j = 0; j < Y.count; ++j) {
                int idx = i * N + j;
                allowed[idx] = uint8_t((X.boxes[i] & Y.boxes[j]) == 0);
                unions[idx] = uint16_t(X.mask[i] | Y.mask[j]);
            }
        }
    }

    void run() { recurse(1, 1); }

private:
    Types X, Y;
    array<int, N> rx{}, ry{};
    array<uint8_t, N * N> allowed{};
    array<uint16_t, N * N> unions{};
    array<uint8_t, 1 << N> union_count{};
    array<array<uint8_t, 2>, C> patterns{};
    array<uint8_t, C> pattern_count{};
    uint64_t scalar_weight_;
    QuotientBuilder& quotient_;
    Counters& counters_;

    bool allow(int i, int j) const noexcept { return allowed[i * N + j] != 0; }

    int ways_row(int i) const {
        array<int, N + 1> dp{};
        dp[0] = 1;
        for (int j = 0; j < Y.count; ++j) if (allow(i, j) && ry[j]) {
            array<int, N + 1> next{};
            for (int s = 0; s <= rx[i]; ++s) if (dp[s]) {
                for (int z = 0; z <= min(ry[j], rx[i] - s); ++z) next[s + z] += dp[s];
            }
            dp = next;
        }
        return dp[rx[i]];
    }

    int ways_col(int j) const {
        array<int, N + 1> dp{};
        dp[0] = 1;
        for (int i = 0; i < X.count; ++i) if (allow(i, j) && rx[i]) {
            array<int, N + 1> next{};
            for (int s = 0; s <= ry[j]; ++s) if (dp[s]) {
                for (int z = 0; z <= min(rx[i], ry[j] - s); ++z) next[s + z] += dp[s];
            }
            dp = next;
        }
        return dp[ry[j]];
    }

    bool feasible() const {
        for (int i = 0; i < X.count; ++i) if (rx[i]) {
            int capacity = 0;
            for (int j = 0; j < Y.count; ++j) if (allow(i, j)) capacity += ry[j];
            if (capacity < rx[i]) return false;
        }
        for (int j = 0; j < Y.count; ++j) if (ry[j]) {
            int capacity = 0;
            for (int i = 0; i < X.count; ++i) if (allow(i, j)) capacity += rx[i];
            if (capacity < ry[j]) return false;
        }
        return true;
    }

    bool apply(int i, int j, int z, uint64_t numerator, uint64_t denominator,
               uint64_t& out_numerator, uint64_t& out_denominator) {
        if (z == 0) {
            out_numerator = numerator;
            out_denominator = denominator;
            return true;
        }
        uint16_t u = unions[i * N + j];
        uint8_t missing = union_missing[u];
        if (missing >= C) {
            cerr << "invalid four-row union mask\n";
            exit(3);
        }
        uint8_t& count = pattern_count[missing];
        if (count + z > 2) return false;

        uint8_t old = union_count[u];
        for (int t = 1; t <= z; ++t) numerator *= uint64_t(old + t);
        if (z == 2) denominator *= 2;
        else if (z > 2) {
            for (int t = 2; t <= z; ++t) denominator *= uint64_t(t);
        }
        union_count[u] = uint8_t(old + z);
        for (int t = 0; t < z; ++t) patterns[missing][count + t] = union_pattern[u];
        count = uint8_t(count + z);
        out_numerator = numerator;
        out_denominator = denominator;
        return true;
    }

    void undo(int i, int j, int z) {
        if (z == 0) return;
        uint16_t u = unions[i * N + j];
        union_count[u] = uint8_t(union_count[u] - z);
        uint8_t missing = union_missing[u];
        pattern_count[missing] = uint8_t(pattern_count[missing] - z);
    }

    void emit(uint64_t numerator, uint64_t denominator) {
        ++counters_.leaves;
        uint64_t key = 0;
        for (int p = 0; p < C; ++p) {
            if (pattern_count[p] != 2) {
                cerr << "bad missing-coordinate degree\n";
                exit(3);
            }
            uint8_t id = encode_pair(patterns[p][0], patterns[p][1]);
            key |= uint64_t(id) << (8 * p);
        }
        if (numerator % denominator != 0) {
            cerr << "nonintegral native contingency coefficient\n";
            exit(3);
        }
        uint64_t kc = numerator / denominator;
        uint16_t response = quotient_.get_or_create(key, counters_);
        quotient_.weights[response] += (unsigned __int128)scalar_weight_ * kc;
    }

    void row_distribution(
        int i,
        const vector<int>& js,
        int pos,
        int need,
        uint64_t numerator,
        uint64_t denominator) {
        if (pos == int(js.size())) {
            if (need == 0) {
                int old = rx[i];
                rx[i] = 0;
                recurse(numerator, denominator);
                rx[i] = old;
            }
            return;
        }
        int j = js[pos];
        int maximum = min(need, ry[j]);
        for (int z = 0; z <= maximum; ++z) {
            ry[j] -= z;
            uint64_t num2 = 0, den2 = 0;
            bool applied = apply(i, j, z, numerator, denominator, num2, den2);
            if (applied) {
                row_distribution(i, js, pos + 1, need - z, num2, den2);
                undo(i, j, z);
            }
            ry[j] += z;
        }
    }

    void col_distribution(
        int j,
        const vector<int>& is,
        int pos,
        int need,
        uint64_t numerator,
        uint64_t denominator) {
        if (pos == int(is.size())) {
            if (need == 0) {
                int old = ry[j];
                ry[j] = 0;
                recurse(numerator, denominator);
                ry[j] = old;
            }
            return;
        }
        int i = is[pos];
        int maximum = min(need, rx[i]);
        for (int z = 0; z <= maximum; ++z) {
            rx[i] -= z;
            uint64_t num2 = 0, den2 = 0;
            bool applied = apply(i, j, z, numerator, denominator, num2, den2);
            if (applied) {
                col_distribution(j, is, pos + 1, need - z, num2, den2);
                undo(i, j, z);
            }
            rx[i] += z;
        }
    }

    void recurse(uint64_t numerator, uint64_t denominator) {
        int total = 0;
        for (int i = 0; i < X.count; ++i) total += rx[i];
        if (total == 0) {
            emit(numerator, denominator);
            return;
        }
        if (!feasible()) return;

        int best_side = -1, best = -1, best_ways = numeric_limits<int>::max();
        for (int i = 0; i < X.count; ++i) if (rx[i]) {
            int w = ways_row(i);
            if (w < best_ways) {
                best_ways = w;
                best = i;
                best_side = 0;
            }
        }
        for (int j = 0; j < Y.count; ++j) if (ry[j]) {
            int w = ways_col(j);
            if (w < best_ways) {
                best_ways = w;
                best = j;
                best_side = 1;
            }
        }
        if (best_ways <= 0) return;

        if (best_side == 0) {
            vector<int> js;
            for (int j = 0; j < Y.count; ++j) if (ry[j] && allow(best, j)) js.push_back(j);
            row_distribution(best, js, 0, rx[best], numerator, denominator);
        } else {
            vector<int> is;
            for (int i = 0; i < X.count; ++i) if (rx[i] && allow(i, best)) is.push_back(i);
            col_distribution(best, is, 0, ry[best], numerator, denominator);
        }
    }
};

static string representative_words(uint64_t qkey) {
    auto words = unpack_q(qkey);
    string s;
    for (int i = 0; i < N; ++i) {
        if (i) s.push_back(' ');
        s += to_string(unsigned(words[i]));
    }
    return s;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    const string triples_path = argc > 1 ? argv[1] : "native_c5_response_quotient_triples.csv";
    const string signatures_path = argc > 2 ? argv[2] : "native_c5_response_quotient_signatures.csv";
    const auto start = chrono::steady_clock::now();
    auto seconds = [&]() { return chrono::duration<double>(chrono::steady_clock::now() - start).count(); };

    generate_group();
    initialize_pair_codes();
    initialize_union_meta();
    cerr << "group=" << G.size() << " setup_seconds=" << seconds() << "\n";

    build_complete_orbit_index();
    cerr << "complete_concrete=" << q_id.size() << " complete_orbits=" << q_orbits.size()
         << " seconds=" << seconds() << "\n";

    vector<Orbit2> two = generate_two_row_orbits();
    cerr << "two_orbits=" << two.size() << " seconds=" << seconds() << "\n";

    // 2^27 slots, 1 GiB, load 0.463 at the verified 62,185,328 raw states.
    QuotientBuilder quotient(27);
    Counters counters;

    for (int i = 0; i < int(two.size()); ++i) {
        const Orbit2& left = two[i];
        for (int j = i; j < int(two.size()); ++j) {
            const Orbit2& right = two[j];

            vector<Config> images;
            images.reserve(G.size());
            for (const auto& g : G) images.push_back(transform_config(right.representative, g));
            sort(images.begin(), images.end());
            images.erase(unique(images.begin(), images.end()), images.end());
            vector<uint8_t> alive(images.size(), 1);

            for (size_t k = 0; k < images.size(); ++k) if (alive[k]) {
                ++counters.placements;
                vector<size_t> orbit_indices;
                orbit_indices.reserve(left.stabilizer_elements.size());
                for (uint16_t hi : left.stabilizer_elements) {
                    Config z = transform_config(images[k], G[hi]);
                    auto it = lower_bound(images.begin(), images.end(), z);
                    if (it == images.end() || !(*it == z)) {
                        cerr << "stabilizer image missing\n";
                        return 3;
                    }
                    orbit_indices.push_back(size_t(it - images.begin()));
                }
                sort(orbit_indices.begin(), orbit_indices.end());
                orbit_indices.erase(unique(orbit_indices.begin(), orbit_indices.end()), orbit_indices.end());
                for (size_t index : orbit_indices) alive[index] = 0;
                uint64_t image_orbit_size = orbit_indices.size();

                // The globally scaled native weight is
                //   |G|/s_x * |H_x . y'| * (2-delta_xy) F2(x)F2(y)
                // = |G|/h_D times the symmetric source weight.
                uint64_t scaled = (GROUP_ORDER / left.stabilizer) * image_orbit_size;
                scaled *= uint64_t(i == j ? 1 : 2) * left.f2 * right.f2;

                ContingencyEnumerator e(left.representative, images[k], scaled, quotient, counters);
                e.run();
            }
        }
        if (i % 5 == 0 || i + 1 == int(two.size())) {
            cerr << "i=" << i
                 << " placements=" << counters.placements
                 << " leaves=" << counters.leaves
                 << " quotient_orbits=" << counters.quotient_misses
                 << " response_classes=" << quotient.responses.size()
                 << " raw_table=" << quotient.raw_to_response.size()
                 << " seconds=" << seconds() << "\n";
        }
    }

    if (counters.leaves != 122166792ULL) {
        cerr << "unexpected contingency leaf count " << counters.leaves << "\n";
        return 4;
    }
    if (counters.placements != 3658027ULL) {
        cerr << "unexpected placement count " << counters.placements << "\n";
        return 4;
    }
    if (counters.quotient_misses != 17120ULL) {
        cerr << "unexpected coordinate-orbit miss count " << counters.quotient_misses << "\n";
        return 4;
    }
    if (quotient.raw_to_response.size() != 62185328ULL) {
        cerr << "unexpected raw four-row table size " << quotient.raw_to_response.size() << "\n";
        return 4;
    }

    vector<unsigned __int128> q_numerator(q_orbits.size(), 0);
    uint64_t response_terms = 0;
    for (size_t r = 0; r < quotient.responses.size(); ++r) {
        const auto& sig = quotient.responses[r];
        for (int k = 0; k < sig.size; ++k) {
            q_numerator[sig.q[k]] += quotient.weights[r] * sig.coefficient[k];
            ++response_terms;
        }
    }
    counters.response_terms = response_terms;

    vector<unsigned __int128> f5(q_orbits.size(), 0);
    cpp_int total = 0;
    for (size_t q = 0; q < q_orbits.size(); ++q) {
        unsigned __int128 scaled = q_numerator[q] * q_orbits[q].stabilizer;
        if (scaled % GROUP_ORDER != 0) {
            cerr << "nonintegral final orbit normalization q=" << q << "\n";
            return 5;
        }
        f5[q] = scaled / GROUP_ORDER;
        cpp_int F = cpp_int(u128str(f5[q]));
        cpp_int weight = cpp_int(q_orbits[q].orbit_size) * q_orbits[q].labelled_multiplicity;
        total += weight * F * F;
    }

    const cpp_int expected("1903816047972624930994913280000");
    if (total != expected) {
        cerr << "N5 mismatch: got " << total << " expected " << expected << "\n";
        return 6;
    }

    ofstream csv(triples_path);
    csv << "qid,representative_words,coordinate_orbit_size,labelled_multiplicity,F5\n";
    vector<size_t> order(q_orbits.size());
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return q_orbits[a].representative < q_orbits[b].representative;
    });
    for (size_t q : order) {
        csv << q << ",\"" << representative_words(q_orbits[q].representative) << "\"," 
            << q_orbits[q].orbit_size << ',' << q_orbits[q].labelled_multiplicity << ','
            << u128str(f5[q]) << '\n';
    }


    ofstream sigcsv(signatures_path);
    sigcsv << "response_id,coordinate_orbit_count,term_count,terms_qid_colon_coefficient\n";
    for (size_t r = 0; r < quotient.responses.size(); ++r) {
        const auto& sig = quotient.responses[r];
        sigcsv << r << ',' << quotient.orbit_counts[r] << ',' << unsigned(sig.size) << ",\"";
        for (int k = 0; k < sig.size; ++k) {
            if (k) sigcsv << ' ';
            sigcsv << sig.q[k] << ':' << sig.coefficient[k];
        }
        sigcsv << "\"\n";
    }

    cout << "C 5\n";
    cout << "group_order " << GROUP_ORDER << "\n";
    cout << "two_row_orbits " << two.size() << "\n";
    cout << "unordered_pairs " << (two.size() * (two.size() + 1) / 2) << "\n";
    cout << "relative_placements " << counters.placements << "\n";
    cout << "contingency_leaves " << counters.leaves << "\n";
    cout << "coordinate_orbit_misses " << counters.quotient_misses << "\n";
    cout << "response_quotient_classes " << quotient.responses.size() << "\n";
    cout << "raw_four_row_keys " << quotient.raw_to_response.size() << "\n";
    cout << "raw_table_capacity " << quotient.raw_to_response.capacity() << "\n";
    cout << "raw_table_bytes " << quotient.raw_to_response.bytes() << "\n";
    cout << "orbit_image_insert_attempts " << counters.orbit_image_insert_attempts << "\n";
    uint64_t merged_response_classes = 0;
    uint16_t max_orbits_per_response = 0;
    for (uint16_t count : quotient.orbit_counts) {
        if (count > 1) ++merged_response_classes;
        max_orbits_per_response = max(max_orbits_per_response, count);
    }
    cout << "signature_completion_records " << counters.signature_completion_records << "\n";
    cout << "response_terms " << counters.response_terms << "\n";
    cout << "response_classes_with_multiple_coordinate_orbits " << merged_response_classes << "\n";
    cout << "max_coordinate_orbits_per_response " << max_orbits_per_response << "\n";
    cout << "raw_lookup_probes " << counters.raw_lookup_probes << "\n";
    cout << "raw_insert_probes " << counters.raw_insert_probes << "\n";
    cout << "N5 " << total << "\n";
    cout << "triples_csv " << triples_path << "\n";
    cout << "signatures_csv " << signatures_path << "\n";
    cout << fixed << setprecision(6) << "seconds " << seconds() << "\n";
    return 0;
}

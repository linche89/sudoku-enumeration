// brute2xC.cpp — direct backtracking counter for 2xC Sudoku grids.
//
// Grid is (2C) x (2C). Boxes are 2 rows x C cols.
//   rows    : 2C rows, each a permutation of {0..2C-1}
//   cols    : 2C cols, each a permutation of {0..2C-1}
//   bands i : rows 2i, 2i+1   (i = 0..C-1)
//   stacks s: cols s*C..s*C+C-1 (s = 0,1)
//   box(i,s): 2 rows x C cols must contain all 2C symbols once.
//
// We fix the FIRST ROW to the identity 0,1,...,2C-1 (factor (2C)!), and at the
// very end multiply the raw count by (2C)!.  This is the standard reduction and
// makes 2x3 brute-forceable.  Fills cell-by-cell in row-major order with bitmask
// pruning on row, column and box.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <string>

static int C, M;             // M = 2C
static int FULL;             // (1<<M)-1
static int grid[16][16];
static int rowMask[16], colMask[16], boxMask[16];   // boxMask indexed by band*2+stack
static unsigned long long count;

static inline int boxId(int r, int c) { return (r/2)*2 + (c/C); }

static void solve(int r, int c) {
    if (r == M) { ++count; return; }
    int nr = (c+1==M) ? r+1 : r;
    int nc = (c+1==M) ? 0   : c+1;
    int b = boxId(r,c);
    int avail = FULL & ~rowMask[r] & ~colMask[c] & ~boxMask[b];
    while (avail) {
        int bit = avail & -avail; avail -= bit;
        rowMask[r]|=bit; colMask[c]|=bit; boxMask[b]|=bit;
        solve(nr,nc);
        rowMask[r]^=bit; colMask[c]^=bit; boxMask[b]^=bit;
    }
}

int main(int argc, char** argv) {
    C = (argc>1)?std::atoi(argv[1]):2;
    M = 2*C; FULL = (1<<M)-1;
    for (int i=0;i<M;++i){ rowMask[i]=colMask[i]=0; }
    for (int i=0;i<M;++i) boxMask[i]=0;
    // fix first row = identity
    for (int c=0;c<M;++c){ int bit=1<<c; grid[0][c]=c; rowMask[0]|=bit; colMask[c]|=bit; boxMask[boxId(0,c)]|=bit; }
    count=0;
    solve(1,0);
    unsigned __int128 fact=1; for(int i=2;i<=M;++i) fact*=(unsigned)i;
    unsigned __int128 total = (unsigned __int128)count * fact;
    auto pr=[](unsigned __int128 x){ std::string s; if(!x)s="0"; while(x){s=char('0'+(int)(x%10))+s;x/=10;} return s; };
    std::printf("2x%d brute: row0-fixed count=%llu  total=%s\n", C, count, pr(total).c_str());
    return 0;
}

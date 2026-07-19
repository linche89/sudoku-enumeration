# Problem B — 2×C 带核 $K$ 的紧致结构（低秩 / 稀疏分解）

> 目的:把"能否绕过 `multiset_q` 的稠密带转移、把 $\bar K^{\,C}$ 算得远快于逐带笛卡尔积"
> 这件事**正式化**,以便协作攻关。本文件不声称任何结论已证;§5 是可证伪的猜想,
> §6 是最难的障碍点,§7 是小 C 判定实验。所有紧致形式**必须**先过 C=3 金标准逐带权重
> (`../history/og2-handoff-legacy.md` §3) 与 C=4=29136487207403520
> 才可信(障碍 O6)。
>
> 记号与 `src/multiset_q.cpp` 对齐:$n=2C$ 个符号,每 stack $C$ 列,$C$ 条带。
>
> 2026-07-20 状态:E2 已完成到预声明的 C=5 下界门槛。C=2--4 显式矩阵和
> 端点全闭合;C=3 中层精确满秩 3,C=5 的 38801 方阵经双素数 CountSketch
> 严格证明 $\operatorname{rank}_{\mathbb Q}\ge1024$。这否决“几百维”B1,
> 但不等于证明满秩 38801。B2/E3 的**固定源、保留完整 target**版本也已由
> 可达 C=6 证人严格否决:任意固定 3+3 split 的线性前沿宽度至少 42,191,464。
> 尚未否决的是先跨 source 求和、只输出实际 target 向量的快速变换。

---

## 1. 精确设定

- 符号 $[n]=\{0,\dots,n-1\}$,$n=2C$。直接 2×C 视角:$C$ 条带(带 $i$ = 第 $2i,2i{+}1$ 行),
  2 个 stack(stack0 = 列 $0..C{-}1$,stack1 = 列 $C..2C{-}1$),盒 = 带 × stack = $2\times C$。
- **骨架** $A_i\in\binom{[n]}{C}$ = 带 $i$ 在 stack0 顶行的 $C$ 个符号。盒约束**强制**带内容:

  | | stack0 | stack1 |
  |--|--|--|
  | 顶行 | $A_i$ | $\bar A_i$ |
  | 底行 | $\bar A_i$ | $A_i$ |

- **单 stack (stack0) 填法**(Z-模型,已 brute 验证 C=3,4):给每个符号 $s$、每条带 $i$ 指定一个列
  $\mathrm{col}(s,i)\in[C]$,使得 (a) 每带每行是双射($A_i\to[C]$ 与 $\bar A_i\to[C]$),
  (b) 每符号 $\mathrm{col}(s,\cdot):[C]\to[C]$ 是双射(每符号每列恰一次)。
- **grade-$k$ 单 stack 组态**:处理完前 $k$ 带后,$\mathrm{xmask}_s^{(k)}=\{\mathrm{col}(s,0),\dots,\mathrm{col}(s,k{-}1)\}\in\binom{[C]}{k}$。
  记 $x=(\mathrm{xmask}_s)_{s\in[n]}$。**列正则**约束:$\forall c\in[C],\ \#\{s:c\in\mathrm{xmask}_s\}=2k$
  (列 $c$ 在前 $k$ 带里已填 $2k$ 个格)。等价地 $\sum_s|\mathrm{xmask}_s|=2kC=kn$。
- $W_k$ = grade-$k$ **符号带标号**组态张成的 $\mathbb{Q}$-向量空间。$e\in W_0$(唯一空态),
  $f\in W_C$(唯一满态,$\mathrm{xmask}_s=[C]\ \forall s$)。

---

## 2. 主恒等式 = 算子乘积

**单 stack 带算子** $M_A:W_k\to W_{k+1}$,$A\in\binom{[n]}{C}$:对组态 $x$,枚举新列——
每个顶符号 $s\in A$ 取 $c_s\notin\mathrm{xmask}_s$ 且 $\{c_s\}_{s\in A}$ 双射到 $[C]$;
每个底符号 $s\in\bar A$ 取 $c_s\notin\mathrm{xmask}_s$ 且 $\{c_s\}_{s\in\bar A}$ 双射到 $[C]$;
令 $x'$ 满足 $\mathrm{xmask}'_s=\mathrm{xmask}_s\cup\{c_s\}$。则 $M_A\,x=\sum_{\text{合法 }(c_s)} x'$。

- $M_A$ **因式化**:顶放置(在 $A$ 上的 permanent)$\perp$ 底放置(在 $\bar A$ 上的 permanent),
  两行独立 $\Rightarrow$ `stack0`$(sk)=\langle f|\,M_{A_{C-1}}\cdots M_{A_0}\,|e\rangle$(取 $f$ 的系数)。

**主恒等式**(已 brute 验证 C=2→288, C=3→28200960):$N=\sum_{sk\in\binom{[n]}{C}^{C}}\mathrm{stack0}(sk)^2$。
在联合空间 $W_k\otimes W_k$ 上,用双线性 + 各带独立求和:

$$\boxed{\;N=\big\langle\,f\otimes f\;\big|\;\bar K_{C-1}\cdots\bar K_1\,K_0\;\big|\;e\otimes e\,\big\rangle,\qquad
K=\sum_{A\in\binom{[n]}{C}}M_A\otimes M_A\;}$$

$K:W_k\otimes W_k\to W_{k+1}\otimes W_{k+1}$ 是**带核**。这就是 `multiset_q` 逐带做的事;
`buildHistFast` 的 $\mathrm{distA}\circledast\mathrm{distB}$ 就是 $M_A\otimes M_A$ 的展开,
$\sum_A$ 就是遍历骨架。**引擎 = 在对称约简空间里稠密地应用 $K$。**

---

## 3. 对称性与下降(关键:为什么必须停在 $K$ 层)

- $S_n$ 置换符号,作用在 $W_k$;有 $M_{\sigma A}=\sigma M_A\sigma^{-1}$。
- **$K$ 是 $S_n$-不变的**:$\sigma K\sigma^{-1}=\sum_A(\sigma M_A\sigma^{-1})^{\otimes2}=\sum_A M_{\sigma A}^{\otimes2}=K$(重排求和)。
  同理 $S_C\times S_C$(每 stack 内列置换)与 $\mathbb{Z}_2$(stack 互换)与 $K$ 对易,且 $e\otimes e,f\otimes f$ 全不变。
- 记 $G=S_n\times(S_C\times S_C)\rtimes\mathbb{Z}_2$。计算**下降**到 $G$-不变子空间 $\bar W_k=(W_k\otimes W_k)^{G}$,
  $\bar K:\bar W_k\to\bar W_{k+1}$。$\bar W_k$ 的一组基 = **规范多重集态**(`canonMS2` 的 key)= 引擎的态。
- ⚠️ **单个 $M_A$ 不下降**(它破坏 $S_n$);只有求和后的 $K$ 下降。任何紧致形式**必须写在 $\bar K$ 层,
  不能写在 $M_A$ 层**。这是障碍 O1/O3 的根。

---

## 4. 维数地板(为什么 B 是唯一的门)

$\dim\bar W_k=\#\{\text{grade-}k\text{ 规范态}\}$。实测(含 swap 口径):

| C | grade 序列 $\dim\bar W_0,\dots,\dim\bar W_C$ |
|--|--|
| 4 | $1,5,141,5,1$(含 swap;无 swap 的中层为 232) |
| 5 | $1,7,38801,38801,7,1$ |
| 6 | $1,11,\ ?,\ ?,\dots$(中点预计百万级) |

**命题(地板)**:任何要表示 $\bar W_k$ 中**任意向量**的显式线性 DP,其转移
向量维为 $\dim\bar W_k$。§3 的等距分解已经提取平凡块,所以单纯更换群表示
或关联方案基底不能再降这个维数。这个命题不排除只作用于实际可达向量的
fast transform,也不排除只保留最终下游响应的专用电路。

⟹ **低于该维数所隐含的成本,当且仅当利用 $\bar K$ 本身的结构**(超出"物化向量"的稀疏性,
或一个分解 $\bar K=\prod(\text{廉价因子})$)。**这就是 Problem B。**

**成本模型**:现在应用一次 $\bar K$(一条带)约 $D\cdot 924\cdot(\text{cross})$,
$D=\dim\bar W_k$,cross $\approx$ 每任务 $10^4$(C=5 实测 12.9k;C=6 预计 3–5 万)。
目标:把逐带成本压到 $\ll D\cdot(\text{cross})$。

---

## 5. Problem B —— 三个可证伪的紧致结构猜想

### B1(谱 / 低秩)
$\mathrm{rank}(\bar K_k)$(作为 $\bar W_k\to\bar W_{k+1}$)被 $\mathrm{poly}(C)$ 界住,或至少 $\ll D$。
若 $\bar K=UV$,$V:\bar W_k\to\mathbb{Q}^r$,$U:\mathbb{Q}^r\to\bar W_{k+1}$,$r$ 小,则
$$\bar K_{C-1}\cdots\bar K_0=U_{C-1}\Big(\prod(V\!U)\Big)V_0,$$
整个 DP 活在 $r$ 维里。**收益:指数级**(把 $D$ 换成 $r$)。
- **可信度:低**。每个源态的目标分布 ~9500-稀疏、跨源几乎不重(O4),行向量看着线性无关。
- **判定 = 实验 E2**:先在 C=2--4 显式建 $\bar K$,再对 C=5 中层做精确模秩下界。
  2026-07-19 已得 C=3 满秩 3 和 C=5 严格下界 1024;见 §7.1。

### B2(初等分解 / 列转移)

需要区分两个不同的输出契约。

**B2a(固定源、区分 target;已否决)**:固定一个 source,按列或
inclusion-exclusion 顺序生成该 source 的完整 target 多项式。对固定 source,
target 由集合差唯一恢复两个 assignment map;不同 extendable prefix 的后缀-
target 支撑互不相交。因此任何线性前沿的宽度至少是 extendable prefix 数。
§7.2 的可达 C=6 平凡稳定子证人给出固定 3+3 split 宽度至少 42,191,464,
terminal support 5,489,549,616。普通列 DP、signed Ryser/Glynn 与 external sort
若仍输出完整 source row,都只是重排同一支撑。

**B2b(target-only、跨 source;仍开放)**:给实际可达 orbit 向量 $x$,直接算

$$y_{[t]}=\sum_{[s]}x_{[s]}\,\bar K_k([t],[s]),$$

并在表示任何 $(s,t)$ pair 或完整 source row 之前跨 source 聚合。它可以满秩;
目标是 fast transform,不是预设低秩。必须给出充分态、精确 orbit normalization、
跨带闭包和复杂度。B2a 下界不覆盖这个输出契约。

### B3(twirl / 换位子 —— 只保留响应子空间)
$\bar K=\sum_A M_A\otimes M_A$ 是 $M_{A_0}\otimes M_{A_0}$ 在 $A_0$ 的 $S_n$-轨道上的**群平均(twirl)**。
twirl 是 $S_n$ 作用的 intertwiner ⟹ 由 Schur 引理在每个 $S_n$-等距分量上是**块标量**。
- **陷阱**:我们已经落在 $S_n$-平凡块里(= 多重集约简),而那个块本身就是 $D$ 维,所以
  "对 $S_n$ 做 twirl" **给不出新东西**。
- **已否决的宽版本**:物化完整 coherent-configuration/orbital algebra。C=6
  两行层有 276 个平凡稳定子 orbit;其 38,226 个无序 pair 单独就有
  $38226\cdot46080=1,761,454,080$ 个 relative-placement coordinates。
  Fourier block 只换基,不减少完整 $\operatorname{Hom}_G$ 维数。
- **仍可能有牙口的版本**:证明实际下游加权响应只落在一个共同的小
  Fourier/communication 子空间,并只构造该子空间。精确问题不是“完整代数是否
  可分块”,而是“所需 response span 的维数和显式生成元是什么”。

---

## 6. 最难的点(障碍,精确陈述)

- **O1(关联而非 profile)**:$M_A$ 的作用依赖 $A$-成员资格与当前 $(\mathrm{xmask},\mathrm{ymask})$ 的
  **联合关联**,不只依赖符号匿名的 profile。只有求和后的 $\bar K$ 下降到匿名空间 ⟹ 紧致形式只能在 $\bar K$ 层找。
- **O2(带内 permanent)**:双射约束把一条带的 $C$ 列全耦合 ⟹ permanent;联合版把 X-双射与 Y-双射
  在**同一顶符号集**上耦合。
- **O3(共享 A 的张量耦合)**:X、Y 共享 $A$ ⟹ 联合态**不能**替换成(X-态)×(Y-态);关联本身就是态。
- **O4(近单射的 merge)**:单任务内 cross → target 只压缩 ≈1.35×(实测)。任何仍**逐 placement 枚举**
  目标的方法都被顶在这个天花板附近。真正的胜利必须**不枚举 placement 就得到 target 权重**。
- **O5(平凡自同构)**:规范态 $|\mathrm{Aut}|\sim O(1)$ ⟹ 骨架/轨道商只有 ≈1.16×。对称性已用尽。
- **O6(精确性闸门)**:任何紧致 $\bar K$ 必须逐带复现 C=3 金标准权重 + C=4/5 OEIS 值(字节级)才可信;
  "看似精确实则偏一点"是最危险的失败模式。
- **O7(固定源支撑地板)**:固定源的完整 target polynomial 在可达 C=6 证人上有
  5,489,549,616 个正系数,且所有 3+3 flattening 都至少秩 42,191,464。下一种
  表示必须在 source 求和之前就避免这个输出契约。

---

## 7. 小 C 判定实验(便宜、可证伪)

- **E1**:枚举**单 stack** 每 grade 的规范态数(C=3,4,5,6)。为 "$S^2\approx D$" 提供地面真值,
  也界定 B1/B2 里 $r,q$ 的下界候选。
- **E2(已完成预声明门槛)**:C=2--4 显式建完整 $\bar K_k$;C=5 对 1024 个完整源行做
  signed target CountSketch,得到严格模秩下界。结果见 §7.1。
- **E3(已被严格下界取代)**:固定源、区分 target 的联合列前沿已由 §7.2
  的可达 C=6 证人否决;不再通过扩大 C=5 probe 重复测试。
- **E4(宽版本已否决)**:完整 orbital algebra 至少含 1,761,454,080 个已知
  regular-block coordinates。只有指定的下游 response subspace 版本仍开放。
- **E5(下一项理论闸门)**:构造 B2b 的 target-only 实际向量变换,或 B3 的共同
  response span。先逐项复现 C=3/C=4 输出,再报告有界 C=5 actual-vector 的维数、
  运算数、时间与 RSS;没有 exact construction 时不启动 C=6 数据实验。
- 全部以复现 C=3 金标准(HANDOFF §3)+ C=4=29136487207403520 为闸门。

### 7.1 E2 verified result (2026-07-19)

原型 `../../experiments/proto/band_kernel_rank.cpp` 的完整小 C 结果为:

| C | 含 swap 的 grade 维数 | 各层精确秩 |
|---:|---:|---:|
| 2 | $1,2,1$ | $1,1$ |
| 3 | $1,3,3,1$ | $1,3,1$ |
| 4 | $1,5,141,5,1$ | $1,5,5,1$ |

C=3 中层 3×3 矩阵有 9 个非零元,精确行列式 2,048,000。C=4 的
5×141 与 141×5 矩阵各有 677 个非零元;21,280 个 side-histogram 与 brute
builder 全一致,端点精确复现 $N(4)$。

C=5 中层为 38801×38801。取 1024 个确定性等距源行,每行做完整转移,
目标侧用固定 seed 的 signed CountSketch。所得 1024 方阵在
$p=1000000007$ 与 $p=1000000009$ 下都满秩,故任一非零模 minor 都严格给出

$$\operatorname{rank}_{\mathbb Q}(\bar K_2)\ge1024.$$

全部 2,084,272,587 个 canonical task-target 项均落在预先由补集生成的
38,801 目标基底中。完整证书、命令、哈希和 RSS 见
`../reports/og2/band-kernel-rank-20260719.md`。

**判定边界**:这排除经 $\le1023$ 维空间的 exact factorization,也排除直接经
C=5 的 126 维 matching-cycle kernel 因式化;但它没有给出 38,801 的精确秩,
也不能排除秩为 1,024 或几千。继续放大 sketch 只提高下界,没有结构上界前不再追数。

### 7.2 Fixed-source frontier certificate (2026-07-20)

原型 `../../experiments/proto/source_target_frontier_bound.cpp` 从两条合法 shared
cut 和四个 assignment vectors 推导一个可达 C=6 grade-2 source。它穷举
$S_6\times S_6\times C_2$,确认 12 个类型互异且 stabilizer 为 1;再用 subset DP、
Ryser 和 balanced-map DFS 三条路径交叉验证:

```text
permanent(B_S) = 4743616    |M(S)| = 74119
permanent(B_T) = 4740096    |M(T)| = 74064
terminal support = 5489549616
3-column prefix minima = 6488 and 6503
fixed 3+3 rank lower bound = 42191464
```

证明使用“完整 target 由 source 与集合差唯一恢复 assignment prefix”,所以不同
prefix 的 completion-target 支撑互不相交。完整证书、命令、哈希和适用边界见
`../reports/og2/source-target-frontier-lower-bound-20260720.md`。

---

## 8. 一句话

换基不能缩小完整 invariant/orbital 空间;E2 排除几百维 B1,E3 的严格证书又
排除固定源完整 target 前沿。现在只剩输出契约更窄的结构性问题:能否对实际
multi-source 向量直接做 target-only fast transform,或只构造共同 downstream
response span。没有这两者之一的 exact construction,继续扩容、换遍历顺序或外排
都不会形成 C=6 路线。

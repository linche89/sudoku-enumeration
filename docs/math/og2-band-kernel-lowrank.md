# Problem B — 2×C 带核 $K$ 的紧致结构（低秩 / 稀疏分解）

> 目的:把"能否绕过 `multiset_q` 的稠密带转移、把 $\bar K^{\,C}$ 算得远快于逐带笛卡尔积"
> 这件事**正式化**,以便协作攻关。本文件不声称任何结论已证;§5 是可证伪的猜想,
> §6 是最难的障碍点,§7 是小 C 判定实验。所有紧致形式**必须**先过 C=3 金标准逐带权重
> (`../history/og2-handoff-legacy.md` §3) 与 C=4=29136487207403520
> 才可信(障碍 O6)。
>
> 记号与 `src/multiset_q.cpp` 对齐:$n=2C$ 个符号,每 stack $C$ 列,$C$ 条带。

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

| C | grade 序列 $\dim\bar W_0,\dots$ |
|--|--|
| 4 | $5,232,5,1$(无 swap) |
| 5 | $7,38801,38801,7,1$ |
| 6 | $11,\ ?,\ ?,\dots$(中点预计百万级) |

**命题(地板)**:任何"物化中间向量"的精确线性 DP 计算 $\langle\bar f|\bar K^{\,C}|\bar e\rangle$,
其转移向量维 $=\dim\bar W_k$。§3 的等距分解**已把唯一有用的平凡块提取出来**(对称 $e,f$ ⟹ 只有 $S_n$-
与 $S_C^2$-平凡分量有贡献),所以群表示/关联方案**不能再降这个维数**。

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
- **判定 = 实验 E2**:C=3,4 上显式建 $\bar K$,算精确秩 + 奇异值衰减。秩随 $C$ 多项式还是 $\sim D$?

### B2(初等分解 / 列转移)
把带算子按**列逐格推进**分解:$M_A=E^{(A)}_{2C-1}\cdots E^{(A)}_0$,每个 $E_j$ 作用在一个
**带内前沿态**(dimension $\le q(C)$)上,从而用 $O(C)$ 个廉价步替代显式 permanent / 笛卡尔积。
- **收益**:把 $\binom{2C}{C}\times(\mathrm{per}\times\mathrm{per})$ 的带算子换成 $O(C)$ 步小转移;
  若联合前沿维保持小,则每带多项式。
- 这是研究日志提过、但**从未为联合 (X,Y) 问题实现**的 KSP 超带 / 列配置转移;
  `buildHistPermDP` 是**单侧**雏形(慢 4×,因为单侧还原不掉笛卡尔积)。
- **难点**:联合的带内前沿(既要 X-双射、又要 Y-双射、还要它们在同一顶符号集上的关联)可能本身就大(O2+O3)。
- **判定 = 实验 E3**:C=3,4,5 测联合带内列转移前沿的最大维。它是 $\mathrm{poly}(C)$ 还是爆炸?

### B3(twirl / 换位子 —— 最深也最投机)
$\bar K=\sum_A M_A\otimes M_A$ 是 $M_{A_0}\otimes M_{A_0}$ 在 $A_0$ 的 $S_n$-轨道上的**群平均(twirl)**。
twirl 是 $S_n$ 作用的 intertwiner ⟹ 由 Schur 引理在每个 $S_n$-等距分量上是**块标量**。
- **陷阱**:我们已经落在 $S_n$-平凡块里(= 多重集约简),而那个块本身就是 $D$ 维,所以
  "对 $S_n$ 做 twirl" **给不出新东西**。
- **可能有牙口的版本**:保持符号**带标号**,换一个**不同的**分解——不是 $S_n$,而是
  组态空间 $\binom{[C]}{k}^{[n]}/\!\sim$ 上的**关联方案 / Gelfand 对**结构(每 stack 的 Johnson 方案
  $J(C,k)$ 的某种积)。精确开问:**$\bar K$ 是否是少数几个方案矩阵的低次多项式?**
  若是,$\bar K$ 有紧致块形,$\bar K^{\,C}$ 可在方案的公共特征基里对角地算。
- **判定 = 实验 E4**:$\bar K_k$ 是否与候选方案矩阵(每 stack 的 $J(C,k)$ 邻接)对易?
  $\bar K$ 是否是它们的低次多项式?

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

---

## 7. 小 C 判定实验(便宜、可证伪)

- **E1**:枚举**单 stack** 每 grade 的规范态数(C=3,4,5,6)。为 "$S^2\approx D$" 提供地面真值,
  也界定 B1/B2 里 $r,q$ 的下界候选。
- **E2(对 B1 决定性)**:C=3($D{=}3$)、C=4($D\le232$)上**显式建 $\bar K_k$ 矩阵**,算精确秩与奇异值。
  秩 $\sim\mathrm{poly}(C)$ ⟹ B1 有戏;秩 $\sim D$ ⟹ B1 死。**这是最该先做的一步。**
- **E3(对 B2)**:C=3,4,5 建联合带内列转移的前沿,测最大前沿维 vs $C$。
- **E4(对 B3)**:测 $\bar K_k$ 与每 stack Johnson 邻接 $J(C,k)$ 的对易性;拟合 $\bar K$ 为方案矩阵的多项式。
- 全部以复现 C=3 金标准(HANDOFF §3)+ C=4=29136487207403520 为闸门。

---

## 8. 一句话

维数地板已证(§4):**换态空间不降维**。唯一的门是 $\bar K$ 的结构(§5)。
最该先做、最能一锤定音的是 **E2**——在 C=3/4 上量出 $\bar K$ 的秩到底是多项式还是 $\Theta(D)$。
秩若多项式,B1 直接给指数级加速;秩若满,B1 死、退守 B2(列转移前沿)与 B3(关联方案),
两者都真难,但至少方向被 E2 钉死,不再盲猜。

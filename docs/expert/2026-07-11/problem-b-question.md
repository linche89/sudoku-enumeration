# Counting 2×C Sudoku via a band-transfer operator: does the kernel have compact (low-rank) structure?

*Self-contained problem statement. No external context required. Notation is defined from scratch. Claims are flagged **[verified]** (checked numerically against known values) or **[conjecture]**.*

---

## 0. The ask, in three sentences

Counting completed 2×C Sudoku grids reduces **exactly** to evaluating $\langle f\,|\,K^{C}\,|\,e\rangle$ for an explicit linear "band kernel" operator $K$ acting on a symmetry-reduced state space of dimension $D$ (millions for $C=6$). The current method applies $K$ densely and hits a wall because $D$ and the per-application cost are both large; a dimension-floor argument (§4) shows no change of state space can reduce $D$. **The open question: does $K$, restricted to the reduced space, have exploitable compact structure — low rank, a short factorization into sparse operators, or a low-degree expression in an association scheme — that lets $K^{C}$ be evaluated in $\ll D\cdot(\text{per-row cost})$ per band?**

---

## 1. The counting problem

Fix $C\ge 2$ and let $n=2C$. A **2×C Sudoku grid** is an $n\times n$ array with symbols $[n]=\{0,\dots,n-1\}$ such that

- every row is a permutation of $[n]$,
- every column is a permutation of $[n]$,
- every **box** contains each symbol once, where the boxes are the $2\times C$ blocks: box $(I,J)$ occupies rows $\{2I,2I{+}1\}$ and columns $\{JC,\dots,JC{+}C{-}1\}$, for $I\in[C]$ (the $C$ **bands**) and $J\in\{0,1\}$ (the $2$ **stacks**).

Let $N(C)$ be the number of such grids. Known values (OEIS **A291187**):

| $C$ | grid | $N(C)$ |
|---:|---|---|
| 2 | 4×4 | $288$ |
| 3 | 6×6 | $28\,200\,960$ |
| 4 | 8×8 | $29\,136\,487\,207\,403\,520$ |
| 5 | 10×10 | $1\,903\,816\,047\,972\,624\,930\,994\,913\,280\,000$ |
| **6** | **12×12** | **unknown — the target** ($\approx 3.8\times10^{49}$) |

$N(6)$ has (to our knowledge) never been published. $N(C)>$ `unsigned __int128` for $C\ge6$, so exact bignum arithmetic is required.

---

## 2. Exact reduction to a transfer operator

All statements in this section are **[verified]** by brute force for $C\le3$ and cross-checked against the table above for $C\le5$.

**2.1 Skeletons.** In band $I$, stack 0 is a box, so its top row (row $2I$, columns $0..C{-}1$) holds some $C$-subset $A_I\subseteq[n]$ and its bottom row holds the complement $\bar A_I$. Because each full row is a permutation of $[n]$, the same band's stack-1 top row holds $\bar A_I$ and its bottom row holds $A_I$. Thus a band is determined, as far as **set** content of its four half-rows goes, by one **skeleton** $A_I\in\binom{[n]}{C}$.

**2.2 Stack factorization.** Fix all skeletons $A_\ast=(A_0,\dots,A_{C-1})$. The number of grids with these skeletons factors over the two stacks, and the two stacks contribute equally, so

$$\boxed{\,N(C)=\sum_{A_\ast\in\binom{[n]}{C}^{\,C}}\ \mathrm{stack}(A_\ast)^2\,}\qquad\textbf{[verified]}$$

where $\mathrm{stack}(A_\ast)$ counts the fillings of **one** stack (a $2C\times C$ region) consistent with $A_\ast$.

**2.3 The single-stack count as a Latin-type constraint.** A filling of one stack assigns, to each symbol $s$ and band $I$, the column $\mathrm{col}(s,I)\in[C]$ it occupies in that band’s two rows of this stack. The constraints are:

- **(row)** for each band $I$: $\{s\mapsto\mathrm{col}(s,I): s\in A_I\}$ is a bijection $A_I\to[C]$, and likewise $\bar A_I\to[C]$;
- **(column)** each stack-column is a permutation of $[n]$, which forces $\mathrm{col}(s,\cdot):[C]\to[C]$ to be a bijection for every symbol $s$ (each symbol appears once per column).

**2.4 Band-by-band state.** Process bands $I=0,1,\dots,C-1$. After $k$ bands the relevant information about a partial single-stack filling is, for each symbol $s$, the set of columns it has used:

$$x_s=\{\mathrm{col}(s,0),\dots,\mathrm{col}(s,k{-}1)\}\in\tbinom{[C]}{k}.$$

Write the state $x=(x_s)_{s\in[n]}$. It is **column-regular**: $\#\{s:c\in x_s\}=2k$ for every column $c\in[C]$ (column $c$ has $2k$ filled cells after $k$ bands). Let $W_k$ be the $\mathbb Q$-vector space with basis the grade-$k$ states. $W_0$ has one basis vector $e$ (all $x_s=\varnothing$); $W_C$ has one basis vector $f$ (all $x_s=[C]$).

**2.5 The single-stack band operator.** For $A\in\binom{[n]}{C}$ define $M_A:W_k\to W_{k+1}$ on a basis state $x$ by

$$M_A\,x=\sum_{(c_s)}\,x',\qquad x'_s=x_s\cup\{c_s\},$$

summed over all choices of a new column $c_s\notin x_s$ for each $s$ such that $\{c_s:s\in A\}$ is a bijection $A\to[C]$ **and** $\{c_s:s\in\bar A\}$ is a bijection $\bar A\to[C]$. (These two bijections are independent — they are the two rows — so $M_A$ is a product of two **permanents** of $C\times C$ 0/1 “allowed-column” matrices.) Then $\mathrm{stack}(A_\ast)=\langle f\,|\,M_{A_{C-1}}\cdots M_{A_0}\,|\,e\rangle$.

**2.6 The band kernel.** Substituting into 2.2 and using bilinearity to sum each band’s skeleton independently:

$$\boxed{\,N(C)=\big\langle\, f\otimes f\ \big|\ K^{\,C}\ \big|\ e\otimes e\,\big\rangle,\qquad
K=\sum_{A\in\binom{[n]}{C}} M_A\otimes M_A\,}\qquad\textbf{[verified]}$$

with $K:W_k\otimes W_k\to W_{k+1}\otimes W_{k+1}$. The tensor square encodes the square in 2.2; the two tensor factors are the two independent fillings $(X,Y)$ forced to **share** the same skeleton sequence. A joint basis state is a pair $(x_s,y_s)_{s\in[n]}$ of column-sets per symbol.

---

## 3. The state space and the computational wall

**3.1 Symmetry reduction.** The group $G=S_n\times(S_C\times S_C)$ acts on joint states ($S_n$ relabels symbols; the two $S_C$’s permute the columns of the two copies), and $K$ commutes with $G$. (There is an extra $\mathbb Z_2$ swapping the two copies.) Both $e\otimes e$ and $f\otimes f$ are $G$-invariant, so the whole computation descends to the space of $G$-invariant vectors, whose basis is the set of $G$-orbits of joint states — the **canonical states**. Their number $D_k$ per grade:

| $C$ | $D_0,D_1,\dots,D_C$ |
|---:|---|
| 4 | $1,\;5,\;232,\;5,\;1$ |
| 5 | $1,\;7,\;76249,\;76249,\;7,\;1$ |
| 6 | $1,\;11,\;?,\;?,\;\dots$ (middle expected $\sim$ millions) |

**3.2 The wall.** The best current engine reduces to canonical states (via a refinement-based graph canonicalization that runs in $O(1)$ relabels on typical reachable states) and applies $K$ one band at a time. Applying $K$ once costs, empirically,

$$\approx D_k\times \tbinom{n}{C}\times(\text{cross}),$$

where $\text{cross}$ is the size of the per-state, per-skeleton convolution $M_A\otimes M_A$ (for $C=5$: $\tbinom{10}{5}=252$ skeletons, $\text{cross}\approx1.3\times10^4$). At $C=5$ the middle band alone is $\sim8.5\times10^6$ “essential (state, skeleton)” tasks and takes $\sim4$ h on 32 threads; the full run reproduces $N(5)$. Extrapolating the three growth factors ($D$: $\sim100\times$; skeletons $252\to924$; $\text{cross}$ larger) puts $C=6$ at **months** of compute plus cache-thrashing memory pressure. Straightforward constant-factor engineering caps out around $2$–$3\times$.

---

## 4. Why the dimension cannot be reduced (the floor)

Since $e\otimes e$ and $f\otimes f$ lie in the $G$-trivial isotypic component and $K$ is $G$-equivariant, $\langle f\otimes f|K^{C}|e\otimes e\rangle$ only ever involves that component. The canonical states are exactly a basis of it. Consequently:

> **Any exact linear method that propagates the state vector uses $\ge D_k$ coordinates, and representation-theoretic / association-scheme block-diagonalization cannot split this further — it is already a single (trivial) isotypic block of dimension $D_k$.**

So the leverage cannot come from a cleverer state space; it can only come from **structure of the operator $K$** restricted to this $D_k$-dimensional block.

---

## 5. The problem, formally

Let $\bar K_k$ denote $K$ restricted to the $G$-invariant subspace, $\bar K_k:\bar W_k\to\bar W_{k+1}$, $\dim\bar W_k=D_k$.

> **Problem B.** Determine whether the operators $\bar K_k$ admit a compact representation enabling evaluation of $\langle f|\bar K_{C-1}\cdots\bar K_0|e\rangle$ in time $\ll D_k\cdot(\text{cross})$ per band. Concretely, decide among the following (in decreasing order of payoff, increasing order of plausibility):

**B1 — Low rank.** Is $\mathrm{rank}_{\mathbb Q}(\bar K_k)$ bounded by $\mathrm{poly}(C)$, or at least $\ll D_k$? If $\bar K_k=U_kV_k$ with inner dimension $r_k\ll D_k$, then

$$\langle f|\bar K_{C-1}\cdots\bar K_0|e\rangle=\langle f|U_{C-1}\Big(\textstyle\prod_k V_{k+1}U_k\Big)V_0|e\rangle$$

runs entirely in the $r_k$-dimensional images — an **exponential** win. *Sanity check:* for $C=4$ the grade sequence $1,5,232,5,1$ already routes the whole computation through dimension-$5$ bottlenecks (grades $1$ and $3$), i.e. $\mathrm{rank}(\bar K)\le5$ everywhere — which is exactly why $C\le4$ is easy. The question is whether this collapse persists: at $C=5$ the middle map $\bar K_2:\ 76249\to76249$ has **no** small-dimension neighbor, so its rank is the decisive unknown.

**B2 — Short sparse factorization.** Does the band operator factor into $O(C)$ **local** steps (e.g. inserting the new band one column/cell at a time), $M_A=E^{(A)}_{2C-1}\cdots E^{(A)}_0$, each $E_j$ acting on a “within-band frontier” of dimension $\le q(C)$, thereby replacing the explicit permanents / cross-product by a short chain of cheap transfers? (This is a KSP-style super-band / column-configuration transfer; it has been built for a **single** side but not for the joint $(X,Y)$ problem.)

**B3 — Association-scheme / commutant structure.** $\bar K=\sum_A M_A\otimes M_A$ is a group-average (twirl) of a fixed $M_{A_0}\otimes M_{A_0}$ over the $S_n$-orbit of $A_0$. Keeping symbols labeled and decomposing under the **column** action, is the joint configuration space a nice association scheme (a product of Johnson schemes $J(C,k)$, one per copy) on which $\bar K$ is a **low-degree polynomial in the scheme’s adjacency matrices**? If so, $\bar K$ is block-diagonalized by the scheme’s common eigenbasis and $\bar K^{C}$ becomes cheap.

---

## 6. Known local structure and the obstructions

**Known transition structure [verified].** For a fixed source state and skeleton $A$, the transition weight factors as $\mathrm{dist}_A\circledast\mathrm{dist}_{\bar A}$ (a convolution under multiset-merge of two single-side histograms). In a coarse coordinate $T[a][b]=|X_a\cap Y_b|$, the update is $T'=T+\Delta_A+\Delta_B$ where each $\Delta$ is a sum of a permutation matrix and “new-symbol vs. old-opposite-side” cross terms. (The coarse $T$ is **insufficient** for $C\ge4$ — it merges distinct orbits — so the full joint multiset is the correct state.)

**Obstructions (the hardest points).**

- **O1 (correlation, not marginals).** A single $M_A$ depends on how $A$ correlates with the current per-symbol column-sets, so it does **not** descend to the symbol-anonymized space; only the summed $K$ does. Any compact form must live at the $K$ level.
- **O2 (within-band permanent).** The bijection constraint couples all $C$ columns of a band (a permanent); the joint version couples the $X$- and $Y$-bijections over the same top-symbol set.
- **O3 (shared skeleton).** Because $X$ and $Y$ share $A$, the joint state cannot be replaced by (state of $X$)×(state of $Y$); the correlation **is** the state.
- **O4 (near-injective merge).** Within one (state, skeleton) task the cross-product compresses to distinct targets by only $\approx1.35\times$ (measured at $C=5$): distinct placements give almost-distinct targets. **Any method that still enumerates one target per placement is capped near this ratio.** A real win must produce target *weights* without per-placement enumeration — which only B1/B3 can, not B2 by itself.
- **O5 (no symmetry left).** Reachable canonical states have automorphism group of size $O(1)$, so orbit/skeleton quotients give only $\approx1.16\times$.
- **O6 (exactness gate).** Any compact $\bar K$ must reproduce the exact per-band weights at $C=3$ and the exact totals at $C=4,5$ before it is trusted; a formula that is “almost right” is the dangerous failure mode.

---

## 7. Falsifiable small-C experiments

- **E1.** Enumerate the single-side reduced state counts $s_k$ (columns-sets multiset mod $S_C$) for $C=3,4,5,6$. Grounds the relation $D_k\lesssim s_k^2$ and gives candidate inner dimensions for B1/B2.
- **E2 (decisive for B1).** Compute $\mathrm{rank}(\bar K_k)$ for the middle grade at $C=5$ — cheaply and exactly via **rank mod a large prime** on the sparse operator (the engine can already apply $\bar K$ to a vector). If the rank is $\ll76249$, B1 is alive (exponential payoff); if it is $\approx76249$, B1 is dead and effort moves to B2/B3. *This is the single most decision-relevant number.*
- **E3 (for B2).** Build the joint within-band column-transfer frontier at $C=3,4,5$; measure its maximum dimension vs. $C$. Polynomial ⇒ B2 viable.
- **E4 (for B3).** Test whether $\bar K_k$ commutes with the per-copy Johnson adjacency matrices $J(C,k)$, and whether $\bar K$ is a low-degree polynomial in them.

All experiments are gated on reproducing the exact per-band weights at $C=3$ and the totals at $C=4,5$ (§1).

---

## 8. Provenance and credit

The reduction in §2 is elementary and self-derived, but it is the specialization to box-height $R=2$ of the band-transfer / gangster method used by Pettersen, Russell, Silver, and “kjellfp” for Sudoku enumeration; priority for that method belongs to them. The contribution sought here is an independent computation of the new value $N(6)$ (and possibly $N(7)$), which hinges entirely on Problem B. A candidate value would be published only after an independent cross-check.

---

*Compact restatement:* $K=\sum_{A\in\binom{[2C]}{C}}M_A\otimes M_A$ is an explicit, $G$-equivariant, column-regular transfer operator with $M_A$ a product of two $C\times C$ permanents. Its symmetry-reduced blocks $\bar K_k$ are what an exact count multiplies together $C$ times. **Is $\bar K_k$ compact (low-rank / short-sparse-factored / scheme-polynomial), and if so, how?**

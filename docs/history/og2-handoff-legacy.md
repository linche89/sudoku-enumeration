> **ARCHIVED:** This handoff records earlier campaigns. Paths and "current"
> claims are historical. Use `../../STATUS.md` and `../runbooks/og2.md` now.

# HANDOFF — 2×C 数独枚举:接力文档

> **给接力 agent**:本文件自包含,假设你**没有**之前的对话上下文。读完即可接手。
> 工作目录:`D:\UGit\sudoku_FJ`(Windows + MinGW g++ 13;`python` = Python 3.13)。
> 详尽时间线见 `docs/OG2_research_log.md`。
>
> ⚠️ **2026-06-30 重大更新(必读,见下方 §A)**:本文件 §0–§7 的核心诊断**部分作废**。
> "瓶颈是代表发现""转移只依赖 T 矩阵"**都是错的**。真相、修复、当前状态全在 §A。
> 旧内容保留作历史与教训,但**以 §A 为准**。
>
> ⚠️⚠️ **2026-07-03 再次重大更新(见 §B)**:§A 的两个核心论断又被实测推翻——
> "对列正则态不存在便宜 canon"(§A.3)**错**,"C=5 瓶颈在 band-1"(§A.4)**错**。
> C=5 战役的真实进展、新引擎 `multiset_q.cpp`、修正后的事实全在 §B。**以 §B 为准**。

---

## §B. 2026-07-03 C=5 复现战役(接力最新状态,取代 §A 的相应结论)

### B.1 快照(2026-07-04 09:41)

- **闸门全过**:C=2=288、C=3=28200960、C=4=29136487207403520,在三个引擎
  (`ms_fast`/`mp_c6`/`mp_q`)上全部精确;`difftest`/`canontest` 0 失配。
- **C=5 已完整复现 OEIS A291187 n=5**:新引擎 `mp_q` 在 2026-07-04 09:41 输出
  `1903816047972624930994913280000 [OK]`。日志 elapsed 为 43647s(12.1h),但其中
  2026-07-04 01:22:54..08:03:38(+08:00) 为系统 sleep/hibernate;扣除后 active elapsed
  约 19603s(5.45h),峰值 RSS 约 16.33GB。
  日志:`data/mp_q-err-20260703-213349.log` / `data/mp_q-out-20260703-213349.log` /
  `data/mp_q-rss-20260703-213349.log`;校准表见 `data/mp_q_c5_calibration_20260704.md`。
- **独立交叉验证在跑**:`mp_fast`(brute lex-min canon,与新 canon 完全独立)band-1
  phase A 实测 **285,371,938 个 distinct raw**,phase B canon 进行中,其 band-1 态数
  必须等于 76249(见 B.2)。它 band-2 起是串行 phase A、数天级——**只用它对账 band-1,别等它出 N**。

### B.2 §A 被推翻的两个论断(重要,防止后人再被误导)

1. **"对列正则态不存在便宜的 canon"(§A.3)——错。**
   旧 refine-canon 零离散化的真实原因:它的细化签名只含**度数级**信息(符号→列颜色直方图)。
   真实 DP 态列正则(每列恰 2d 符号、每符号每侧恰 d 列)⟹ 初始单色着色是不动点,细化**永远启动不了**。
   这是 1-WL 在正则结构上的经典失效,不是"问题本质难"。
   **修复**:用跨栈关联矩阵 $T[a][b]=\#\{s: a\in xmask_s \wedge b\in ymask_s\}$ 和同侧共现矩阵
   $U[a][a']$、$V[b][b']$ 做**初始着色播种**,再迭代细化(签名=颜色标注的 T/U/V 行谱)。
   真实态平均 |Aut| 只有个位数 ⟹ 几乎全部离散化,canon 从 $(C!)^2=14400$ 次重标降到 O(1) 次。
   已实装于 `multiset_c6.cpp` 与 `multiset_q.cpp` 的 `refineColours`。
2. **"C=5 band-1 坍缩成几百状态、band-1 是瓶颈"(§A.4)——错。**
   实测:**C=5 band-1 = 76249 个规范态**(`mp_c6` 无 swap 测得 76249;`mp_q` 在含栈交换的
   扩群下测得 38801,且 2×38801−76249=1353=自对称态数,两者自洽)。
   distinct raw = 285,371,938(`mp_fast` phase A 实测)。
   ⟹ **真墙是 band-2**:76k 源态 × 252 骨架 ≈ 977 万 (态,骨架) 任务,每任务一个
   hTop×hBot 交叉积 + 若干 canon。旧 profiling(`profile_c6.cpp`)从未跑到 band-2,漏掉了它。
   band-3 便宜(depth 3→4 每符号只剩 2 列可选,直方图极小),band-4 平凡。

### B.3 正确性的结构性论证(以后改 canon 全靠它兜底)

本仓库所有新 canon 的 key 都是"**在显式枚举的置换子集上搜出的最小重标副本**"
(细化颜色类内全排列 × 可选栈交换)。因此:
- **key 相等 ⟹ 两输入同轨道**(key 本身就是轨道成员)⟹ **欠合并在构造上不可能**;
- 细化弱只会**过分裂**(同轨道拿到不同 key)⟹ 状态变多、变慢,但 **N 仍精确**
  (任何代表的转移分布都正确);
- 唯一需要测试的性质是**重标不变性**(护性能),以及闸门值(护端到端)。
已测:不变性 4 万例×3 重标(含列正则平衡态=最难情形)0 违例;C≤4 与 brute canon
分离性逐一致(key 数相等、0 冲突);C=2/3/4 N 精确。

### B.4 新主引擎 `src/multiset_q.cpp`(= `build/mp_q.exe`)

在 `multiset_c6` 的流式结构上叠三个**精确**约简 + 三个工程修复:

1. **栈交换对称 X↔Y**(数学,×2):联合 DP 的两份 stack0 拷贝可交换,状态群从
   $S_C{\times}S_C$ 扩到 $(S_C{\times}S_C)\rtimes\mathbb{Z}_2$。band-1 态数 76249→38801。
   canon 取两分支(原/交换)的 min。闸门过。
2. **骨架商 / 打标签 canon**(§5 的老路线 3,数学,实测仅 ×1.16):把 (源态,骨架A) 按
   "top 符号打 TAG 位后的规范形"分组,同类转移分布只算一次。收益小的原因:真实态
   |Aut| 太小,252 个骨架在 Aut 作用下几乎不合并。保留(无害),但**别指望它救命**。
3. **全局无锁 raw→canon 缓存**(工程,大头):canon 是纯函数,band-2 的 ~10^10 次交叉
   积对撞到的 distinct raw 只有 ~10^8 量级。开放寻址表(默认 2^28 槽 ≈15GB,CLI 可调:
   `mp_q 5 <log2cap>`),原子发布,**全 24 字节 key 比对**(指纹碰撞无正确性风险),
   探测失败就地重算(退化只伤速度)。跨 band 复用(纯函数,无需清空)。
4. **工程教训(Windows/MinGW + OpenMP 32 线程,血泪)**:
   - `unordered_map::clear()` 不缩桶数组 ⟹ 复用的大 map 之后每次遍历都按**历史最大桶数**
     付费——这是 `mp_c6` band-2 只有 ~434 任务/s 的元凶。
   - **Windows CRT malloc 有全局锁**:canon 热路径里的 `std::vector` 分配在 32 线程下把
     ~6µs 的 canon 拖到 ~100µs。canon/细化路径已全部改为栈上无分配
     (`ClassPerms` odometer、`FSig` 定长签名)。
   - `Big::addMul` 每次调用堆分配 ⟹ 先按 (任务,目标态) 用 uint64 聚合
     (单任务总质量 ≤ $(C!)^4\approx 2{\times}10^8$,无溢出),再一次 Big 运算。

### B.4b 2026-07-03 09:53 崩溃事故(已根因定位、已修复)

- 早晨的 C=5 跑在 band-2 37%(3.15M/8.46M,el≈5952s)处 **APPCRASH 0xc0000005**
  (Windows 事件日志 Application/1000,故障模块 mp_q.exe+0x12e57)。
- **根因 = use-after-free**:phase-2 的 per-thread `sideMemo` 容量上限清理(`clear()`)写在
  `getSide()` **内部**;当一个任务先取得 `hTop` 引用、再取 `hBot` 时恰好触发 cap 清理,
  `hTop` 悬空 → 交叉积循环读已释放内存。每线程 distinct 侧数在 band-2 深处才会越过
  200k 上限——所以 C≤4 闸门和浅探针永远打不中它,只有跑到 band-2 一小时后才炸。
- **修复**:cap 检查移到任务开头(取任何引用之前);任务内 memo 只增不清
  (node-based map,rehash 不搬节点,引用稳定)。C=2/3/4 闸门修复后复验精确。
- **教训**:带容量上限的缓存,清理点必须与"借出引用的生命周期"隔离;
  "闸门全过"护不住只在深水区触发的内存错误,长跑前应做一次小规模 ASan/-D_GLIBCXX_ASSERTIONS 压测。

### B.5 当前在跑的东西(接力者先看这里)

- **`mp_q 5` 已完成**(2026-07-03 21:33 启动,watch_rss 110GB 看门狗):
  终点:`C=5: N=1903816047972624930994913280000  expect [OK]`。实测 band-0=3s,
  band-1=98s,band-2=15635s active,band-3=3865s,band-4=4s;真墙仍是 band-2,但
  旧的 band-2=39777s 包含 24044s 系统休眠,不可用于 ETA。band-2 平均约 541 essential/s;
  先前所谓 band-2 末段掉速主要是休眠伪影。
- ⚠️ 第一次跑(00:55 启动)与 `mp_fast 5` 交叉验证跑都在 01:17 被会话中断**连带杀掉**
  (教训:后台长跑必须用 `Start-Process` detach,别挂在 agent shell 树下)。
  `mp_fast` 死于 band-1 phase B 149.7M/285.4M canon 处;它的 band-1=76249 独立对账**未完成**,
  是可选补做项(约 2-3h)。注意 `mp_c6`(T-播种 canon、无 swap)已独立测得 76249,
  与 `mp_q` 扩群口径 38801 通过自对称态计数 2×38801−76249=1353≥0 自洽。
- 重启命令(detached,会话安全):
  `Start-Process -FilePath powershell -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-File','scripts\watch_rss.ps1','-Exe','.\build\mp_q.exe','-Arguments','5','-LimitGB','110','-IntervalSeconds','30' -WorkingDirectory 'E:\Code\sudoku_FJ' -WindowStyle Hidden`

### B.6 band-2 后续杠杆(按性价比排序)

1. **带序反转对偶(中间相遇,已实装于 `mp_q dual`)**:倒序处理带 = 合法网格的双射
   (整带置换保持约束)⟹ 反向权重 $w_b[v]=w_f[\bar v]$($\bar v$=逐符号 mask 取补)。
   注意当前 DP 是 quotient 权重,combine 必须除以规范态代表的具体标号状态数
   `Q(k)=(2*C!*C!/aut(k))*((2C)!/prod multiplicity(pair)!)`;补集也必须重新 `canonMS2`。
   C=2/3/4 普通与 `dual` 已同值通过。含栈交换的 `mp_q` 口径下 C=4 中点态数是 141;
   旧文档的 232 是无 X/Y 栈交换口径。对 C=5 的已完成 normal run,扣除系统休眠后
   band-3+4 约占 active time 的 19.7%,所以 dual 是有用的常数加速,但不是绕过
   15,635s band-2 主墙的分钟级解法。
2. **逐带 checkpoint + resume(已实装)**:`mp_q` 默认每完成一个 band 写
   `data/mp_q_C{C}_band{band}.chk`;显式传 `resume` 会从目标范围内最新 checkpoint 恢复。
   对奇数 C 的 `dual resume`,还会读取浅中点 snapshot。
3. **band transition profiling + sideglobal 探针(已实装)**:传 `profile` 会打印 side memo
   命中/清理、hist 大小、cross-product、target 数、raw-canon cache 命中率。传 `sideglobal`
   会先按 band 收集 unique side key 并预计算 side hist;超过 `sideglobal=N` cap 时自动退回
   per-thread memo。该路径是探针/优化用,还需 C=5/C=6 bounded 数据判断是否默认启用。
4. **kernel audit 结果(2026-07-04,见 `data/kernel_audit_20260704.md`)**:
   C=5 band-2 前 20 万 task 中 `uniqueSidePair=200000`,`uniqueHistPair=199995`;
   完全相同 side/hist-pair 复用几乎为零,简单 kernel cache 不是 10×–100× 杠杆。
   `phiaudit` 也证伪了 multiplicity/T/TUV/autQ 直接替代 `phi(canon(target))`:C=5 的 TUV
   仍有 4407 个 collision groups。后续不能靠这些粗 invariant 直接去 canon。
5. **band-2 workbench 结果(2026-07-05,见 `data/band2_optimization_20260704.md`)**:
   `crossfloor` 显示 C=5 band-2 的 raw merge 地板很低:20k task、约 2.51e8 raw merges,
   中段 wall 仅 0.189s;真正的墙是 `raw target -> orbit/phiIndex` 的精确分类。
   `scalarindex` 是当前最好的 scalar oracle 原型;`scalarsigkey` 固定缓冲签名 C=3 全量
   `28200960 [OK]`,C=5 20k sample middle wall 33.5s,正确但没有实质超过 `scalarindex`。
   结论:不要再押注 heap/string refine signature;下一步要么让分类器复用 `canonMS2` 已算数据,
   要么换状态空间。
6. **交叉积成本地板的进一步压缩(降级为备选)**:group/orbit kernel 仍可能有价值,但如果它
   仍需逐 raw 做精确分类,就不是 10× 路线。可研究方向是把 hist pair 压到
   `S_C×S_C×swap` 下的 orbit representative + relative double-coset 数据;或把单侧
   restricted-permutation hist 改写成 rook-polynomial / permanent 生成函数。
7. **B-route permanent DP 原型(已实装旁路,见 `data/b_route_permdp_20260704.md`)**:
   `buildHistPermDP` 把单侧 side histogram 写成彩色 restricted permanent 的 column-subset DP。
   C=3/4/5 随机差分全部 OK,但作为 `buildHistFast` 的 drop-in 替换慢 3.5–4.1×;
   因此不要接主路径。它的价值是作为 fused direct kernel 的地基:
   `top side DP × bot side DP -> target-count vector / dual scalar`,目标是绕过 `hTop×hBot`。
8. **换状态空间(kjellfp a-向量,§A.4b 原建议)**:仍开放,工程量大,C=6 才值得。

### B.7 接力检查清单

```powershell
# 构建
g++ -O3 -mpopcnt -fopenmp -std=c++20 -I src src/multiset_q.cpp -o build/mp_q.exe
# 闸门(改任何东西都必须全过)
.\build\mp_q.exe 2 20 ; .\build\mp_q.exe 3 20 ; .\build\mp_q.exe 4 20
.\build\mp_q.exe 2 dual 20 ; .\build\mp_q.exe 3 dual 20 ; .\build\mp_q.exe 4 dual 20
.\build\mp_q.exe 3 canontest 2000 ; .\build\mp_q.exe 4 canontest 2000 ; .\build\mp_q.exe 3 difftest 500
# C=5(看门狗下)
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\watch_rss.ps1 -Exe .\build\mp_q.exe -Arguments @('5','20','dual','profile','sideglobal=1000000') -LimitGB 110 -IntervalSeconds 10 -MaxMinutes 60
```

C=5 各带参考数据(2026-07-04 实测,`mp_q` 扩群口径):逐带状态数
`7 / 38801 / 38801 / 7 / 1`;band-1 无 swap 口径 76249,distinct raw 285,371,938;
band-2/band-3 essential 任务均为 8,458,157(raw 态×骨架 = 9,777,852),扣除系统休眠后
band-2=15,635s、band-3=3,865s,band-3 每任务约便宜 4.0 倍。

---

## §A. 当前真实状态(2026-06-30,取代旧 §0)

### A.1 已确证的事实链(带独立交叉验证)

1. **真正的性能墙不是"代表发现"**,而是 `src/final2xC.cpp` 里 `canonT` 用 `std::string` 做 key:
   C×C=16 字节超过 libstdc++ 的 15 字节 SSO 缓冲 ⟹ band-0 的 ~2300 万次临时 key 全部堆分配。
   改成栈缓冲 + memcmp 后,C=4 从"跑不完"→ 56s。**这是纯工程,与数学无关。**

2. **速度解决后,`final2xC` 在 C=4 给出错误答案** `29971601054760960`(真值 `29136487207403520`,
   由完全独立的转置 oracle `src/sudoku_rc.cpp` 确认)。**根因是数学建模错误,不是 bug**:
   - 状态 `T[a][b]=|X_a∩Y_b|` 是**不充分统计量**——它只数"多少符号共现",丢失"是哪些符号"。
   - C=3 时 T 恰好等价于真轨道;**C=4 起 T-canon 把不同的联合轨道合并 ⟹ 多算**。
   - 旧文档"单 stack 每带唯一""转移只依赖 T"**只在 C=3 验证过,C=4 假**。
     (实测:C=4 band-1 真轨道数 = **232**,而 T-canon 错误地只给 68。)

3. **正确的状态(已验证 C=2/3/4)**:**符号匿名的 (xmask,ymask) 多重集**,在 $S_C\times S_C$
   (置换 X 列、置换 Y 列)下规范化。xmask=该符号所在的 X 列集合,ymask=Y 列集合。
   这正是 $S_{2C}\times S_C\times S_C$ 下的精确联合轨道(= 慢 oracle `jointcanon.cpp`),
   但只花 $(C!)^2$ 而非 $(2C)!$。转移仍因式分解为 topPart ⊎ botPart(不相交卷积)。

### A.2 当前引擎与验证闸门

- **`src/multiset_fast.cpp`** ← 单线程正确引擎。正确状态 + orbit-aggregated 卷积直方图
  (把每侧 $(C!)^2$ 的双射枚举换成按旧-pair 分组的组合计数;**11.6 万随机例差分测试与 brute 字节一致**)。
- **`src/multiset_par.cpp`** ← **并行主引擎(OpenMP, lex-min canon)**。band 循环 = phase A(串行累加
  全局去重 rawW)/ B(并行 canon)/ C(归约)。编译 `g++ -O3 -mpopcnt -fopenmp -std=c++20 -I src`。
  C=4 = ~1s(24 线程)。⚠️ **内存**:phase A 的全局 rawW 在 C=5 会涨到 ~十几 GB(64G 可容);
  **务必在内存看门狗下跑**(见 A.6)。
- **`src/multiset_refine.cpp`** ← 同引擎但 canon 换成 refinement-key(见 A.3),C=3/C=4 已验证正确。
- **验证(全部通过)**:C=2=288,C=3=28200960(逐带 3,3,1),C=4=29136487207403520(逐带 5,232,5,1)。
- **独立 oracle**:`src/sudoku_rc.cpp`(转置法 $\sum_\sigma B(\sigma)^2$,与转移法完全独立)复现 C=3,C=4。
- **金标准护栏**:见旧 §3 的 C=3 逐带权重表(改任何代码必须仍复现)。

### A.3 canon 的真相(2026-06-30 大量实验 + profiling 后,**已定性**)

- **lex-min canon 本质上贵**:`canonMS` 的 popcount-LB 分支限界在"每符号 popcount 相同"(每带都如此)
  时**永不剪枝**,退化成全 $(C!)^2$。增量 B&B(`src/canon_ir2.cpp`,差分测试 0 失配)也只快 2.2×(C=5)
  /2.8×(C=6)——**lex-min 不是 C=6 的杠杆**。
- **refine-canon 忠实但对真实态无用(决定性负结果)**:`src/canon_refine2.cpp` 用等价划分细化,
  **已验证忠实**(不变性 + 与 brute 精确分离,#keys 相等 0 过度合并)。**但** `experiments/proto/refprof.cpp` 实测:
  **真实 DP 状态是列正则的**(depth d 时每列恰好 2d 个符号)⟹ 极度对称 ⟹ 细化**零离散化**,
  残余搜索 = 满 $(C!)^2$,**100% 的真实态都如此**(C=4 576/576,C=5 14400/14400,C=6 518400/518400)。
  之前随机态能离散化 55% 是**误导**——真实态是最坏情况。
- **⟹ 铁的结论:对列正则态不存在便宜的 canon。canon 就是 ~$(C!)^2$/次,没有捷径。**
  这也解释了为何此问题难、为何 C=6 从未在家用机完成。

### A.4 真正的墙 = canon 不可约 × 状态数;捷径已穷举排除

- profiling(`src/profile_c6.cpp`):C=5 band-1 = 1147 classes × 平均 1364² ≈ **20 亿** rawPair,
  坍缩成几百状态(C=4 是 671748 raw → 232 state,2900:1)。但坍缩**只能通过 canon 实现**,无法提前预测。
- **所有"绕过 canon"的聚合捷径都已差分测试排除**(`experiments/proto/aggtest.cpp`, `experiments/proto/aggderive.cpp`):
  - 单边形状聚合(固定 top 代表 + 全 bot):**错**(丢相对对齐)。
  - 联合轨道(对角 G)聚合:**正确但 ~0 坍缩**(899/900 对各自独立轨道)——无加速。
  - GPU:**不适用**(瓶颈是组合去重 + 图同构 canon,不是稠密数值吞吐)。
- **⟹ 唯一真实杠杆 = 常数级 canon 加速 + 并行(24×) + 机时。**

### A.4b 接力的真正机会(开放问题,诚实)

我们的 (xmask,ymask) 多重集状态**正确但代价是 per-state 图规范化**($(C!)^2$)。
**Pettersen/kjellfp 能算大盘,几乎肯定用的是另一种状态——kjellfp 的 a-向量**
(`a: E(L,R)→ℕ`,记"恰出现在哪几行的符号数",见 `docs/OG2_research_log.md` 的 kjellfp 规格),
它的转移是**容斥/多项式因子**,**根本不做图规范化**,所以没有 $(C!)^2$ 墙。
- **最有希望的下一步**:回到 kjellfp a-向量内核(R=2 特化),重新实现整网格的 C 重列卷积,
  绕开我们这套需要图 canon 的多重集状态。这是**换状态空间**,不是优化现有 canon。
- 风险:a-向量内核我们曾复现失败过一次(漏约束 ii,后修正,见研究日志);需 verbatim 原帖 + 逐级卡死。
- **C=6 提交 OEIS 前**:第二套独立方法或更高精度复算交叉验证(诚实:站在 Pettersen/kjellfp 肩上,
  贡献 = 独立推导/实现 + 可能的新数值项)。

### A.4c 当前可立即跑出的(诚实 ETA)

- **C=5**:`mp_fast`(全局去重,~15-20GB,必带 memwatch)或 `mp_c6`(流式,~MB,但更慢)。
  band-1 是瓶颈(20 亿 rawPair × $(C!)^2$ canon),**实测数小时级**,但内存安全、可出
  `1903816047972624930994913280000` 作跳板验证。
- **C=6**:用现有引擎是**数天~数周**家用机机时。不假装更快。

### A.6 内存安全(血泪教训,2026-06-30)

- **64G 曾被撑爆、系统无响应**:并行版给 24 线程各开一个近乎全量的 thread-local rawW 表 ⟹ ~24× 膨胀。
- **铁律**:并行只复制**小**结构(规范状态表,几百项),**绝不**复制大 raw 表。全局 rawW 只留一份。
- **跑大 C 必须带内存看门狗**:`/tmp/memwatch.sh <exe> <C>` 实时采样 RSS,超 58G 自动 kill。
- 流式版(`multiset_par.cpp` 的 commit 3d9f4b8 历史版)内存有界(~MB)但丢全局去重 ⟹ **太慢**(C=5 band1 ~小时级)。
  现用版 = 全局去重单 rawW + 并行 canon = 快且 64G 可容。

### A.5 已死的路(别重试,全部差分测试/profiling 否决过)

- T-矩阵状态(`final2xC.cpp`/`quotient2xC.cpp`/`conv2xC.cpp`/`tmatrix.cpp`):**C≥4 多算,作废**。
- 对齐法(Y=σ·X 从单个固定 X):**C=4 只覆盖 68/232 个状态就崩**,"单 stack 唯一"在 C≥4 不成立。
- 商转移矩阵跨带摊销:**不成立**——每带状态 popcount 不同,状态在带间不复用。
- WL-canon / 列签名排序快-canon:**差分测试否决**(签名顺序 ≠ packed lex-min 顺序)。
- refine-canon 加速真实态:**0 离散化**(列正则态最坏情况),忠实但无速度收益。
- 单边形状聚合 / 联合轨道聚合:前者错(丢对齐),后者对但 0 坍缩(`experiments/proto/aggderive.cpp`)。
- GPU offload:不适用(组合去重 + 图 canon,非稠密数值)。

### A.7 引擎文件速查(接力直接用哪个)

- **`src/multiset_fast.cpp`** — 单线程参考实现,正确,带 `difftest`/`canontest` 自检。读它理解算法。
- **`src/multiset_par.cpp`**(= `mp_fast`)— **并行 + 全局去重 + lex-min canon**。跑 C≤5 的首选。
  内存 C=5 ~15-20GB,**必须 `/tmp/memwatch.sh ./build/mp_fast.exe 5`**。
- **`src/multiset_c6.cpp`**(= `mp_c6`)— **流式(内存有界 ~MB)+ refine-canon + 有界缓存**。
  内存绝不爆,但更慢;给 C=6 这种"内存放不下全局 rawW"的场景。
- **`src/profile_c6.cpp`** — 带 `MAXBAND`/计数器的探针,诊断用。
- oracle / 验证:`src/sudoku_rc.cpp`(独立转置法),`src/jointcanon.cpp`/`src/exact2xC.cpp`(慢全对称 canon)。
- 编译:`g++ -O3 -mpopcnt -std=c++20 -I src <f>`;并行加 `-fopenmp`;**绝不 `-march=native`**。
- 验证闸门(改任何东西先跑):C=2=288,C=3=28200960(逐带 3,3,1),C=4=29136487207403520(逐带 5,232,5,1)。

---

## 0.(旧)TL;DR —— ⚠️ 已被 §A 取代,保留作历史

- **大目标**:精确计算 $N(2{\times}C)$ = 盒为 $2\times C$ 的 $(2C)\times(2C)$ 数独终局数。OEIS **A291187** 只到 $C=5$;**$C=6$(12×12)从未有人公布精确值** = 我们的真目标(新 OEIS 项)。
- **已完全做到且验证**:9×9 主项(三法 + 自洽 + 理论边界,见 `FJ_sudoku.md`);2×C 的**完整数学方法**(结构定理 + 卷积转移核),在 **C=2,C=3 上逐带 brute 卡死**;一个**单元测试过的 bignum**。
- ~~**唯一卡住的**:把 C=4 跑出来~~ ⟵ **错**:见 §A,C=4 已跑通且修正了状态定义。
- ~~**接力第一步**:打开 `src/final2xC.cpp`,优化"代表发现"~~ ⟵ **作废**:`final2xC` 状态错误,用 `multiset_fast.cpp`。

---

## 1. 问题精确定义

$N(2{\times}C)$:$(2C)\times(2C)$ 网格,符号 $\{0..2C-1\}$,每行/每列是排列,每个 $2\times C$ 盒含全部 $2C$ 符号一次。

**已知值(全部用作验证闸门,OEIS A291187):**
| C | 网格 | N(2×C) |
|--|--|--|
| 2 | 4×4 | 288 |
| 3 | 6×6 | 28200960 |
| 4 | 8×8 | 29136487207403520 |
| 5 | 10×10 | 1903816047972624930994913280000 |
| **6** | **12×12** | **未知 ← 目标** |
| 7 | 14×14 | 未知 |

$N(2{\times}6)\approx 10^{50}$ —— **超出 `unsigned __int128`(上限 ~3.4×10³⁸),必须用 bignum**(已备好,见 §4)。

---

## 2. 已验证的数学(全部 brute 卡死过,可信)

经转置 $\#(2{\times}C)=\#(C{\times}2)$,网格 = 2 条带,列约束耦合 ⟹

**(a) 结构定理**:$N=\sum_{\text{骨架序列}}\text{stack0}^2$,组织成**逐带联合转移**:C 条带,每带选一个**共享骨架** $A$(一个 $C$-子集),在两个副本 X、Y 各自放置;终态两副本列全满。

**(b) 状态 = 跨 stack 交集矩阵** $T[a][b]=|X_a\cap Y_b|$,在 $S_C\times S_C$(行列置换)下规范化。**状态数极少**(C=2:每带 2,1;C=3:每带 3,3,1)。已证**转移只依赖 T**(与具体代表无关)。

**(c) 卷积转移核**(已 brute 验证 54/54,含非空带):
$$T'=T_{\text{base}}+\Delta_A+\Delta_B,\quad \text{weight}=\text{dist}_A \circledast \text{dist}_B$$
- $\Delta_A$(top/A 侧)、$\Delta_B$(bot/~A 侧)**相互独立**;各侧枚举 $(C!)^2$ 个双射对建直方图,再卷积。把 $(C!)^4$ 降到 $2(C!)^2$ + 卷积。
- **每侧 $\Delta$ 含全部交叉项**(关键,曾在此栽过 bug):新符号同时进 X'列 $a_x$、Y'列 $a_y$ → $\Delta[a_x][a_y]{+}1$;新 X-符号撞旧 $Y_b$ → $\Delta[a_x][b]{+}1$;新 Y-符号撞旧 $X_a$ → $\Delta[a][a_y]{+}1$。
- 实现见 `src/final2xC.cpp` 的 `sideDist()`。**已验证:C=3 逐带权重 = 金标准**(见 §3)。

**(d) 单 stack 唯一**:单个 stack 每带只有 1 个 canonical 状态(已测 C=2,3)。⟹ 联合态完全由 X、Y 列的对齐决定。

---

## 3. 金标准(C=3 逐带 state→weight,接力必须保持)

任何对 `final2xC.cpp` 的修改,**必须**仍复现下表(`./build/final2xC.exe 3 dump`)。这是防"代码 bug 反污染推导"的护栏:
```
band0  002020200 -> 4320     002110110 -> 12960    011101110 -> 8640
band1  224242422 -> 760320   224332332 -> 3525120  233323332 -> 4008960
band2  666666666 -> 28200960
C=3: N=28200960  [OK]
```
(key 是 canonical T 矩阵按行展开的字节串;数字是 Big 权重。)

---

## 4. 代码地图(哪个文件干什么)

**第一部分(9×9,已完成,master 上 tag v1.0/v1.1/v1.2):**
- `src/main.cpp` `reduce.hpp` `count.hpp` `band.hpp` — 枚举法基线(N0,344s)
- `src/combinatorial2.cpp` — 9×9 纯组合法,**亚秒级**,`./build/combinatorial2.exe` 出 N0
- `src/combinatorial3.cpp` — 自洽版(只用 B(σ))

**第二部分(2×C,进行中):**
- **`src/final2xC.cpp`** ← **接力主战场**。卷积转移 + bignum + 代表发现。C=2,3 ✅,C=4 卡在代表发现。
- `src/big.hpp` — bignum,**单元测试过**(`build/bigtest.exe`)。`addMul`/`+=`/`str`/`fromDec`。
- `src/jointcanon.cpp` — 慢但正确的 oracle(全 $S_{2C}$ canon),C=2,3 验证用。
- `src/tmatrix.cpp` `conv2xC.cpp` `quotient2xC.cpp` — 迭代过程版本(可参考,非最终)。
- `src/sudoku_rc.cpp` — 早期暴力 2×C(到 2×4),独立 oracle。
- `src/band_count.cpp` — kjellfp 通用带计数,**双闭式验证**(R=2:$(2C)!(C!)^2$;R=3:Franel)。**注意**:这算"一阶矩"$\sum B$(带数),不是网格数 $\sum B^2$;别混淆(见 §6 教训)。

**编译命令(统一)**:`g++ -O3 -mpopcnt -std=c++20 -I src <file> -o build/<exe>`
⚠️ **绝不要 `-march=native`**(本机 MinGW 会 AVX 段错误;见 `memory/mingw-march-native-avx-crash`)。

---

## 5. 待解问题:formulate 清楚的"代表发现"

**问题**:逐带 DP 中,每个新的 canonical 状态 $T'$ 需要**一个具体代表** $(X,Y)$(列内容),用来喂下一带的 `sideDist`(卷积只需 (X,Y) 的列内容)。状态极少(几个),但当前找代表的两种实现都在 C=4 偏慢:
- **placement 叉积**:对每源态×每骨架枚举 X-placements × Y-placements($\sim(C!)^2\times(C!)^2$),C=4 band0→1 ≈ 10⁸ 次 `keyXY`,**卡住**。
- **对齐枚举**:固定单 stack 代表 X,Y 取 X 的全部 $(2C)!$ 符号重标 → C=4 = 8.5s,C=5=10!≈13min,C=6=12! 不可行。

**关键已知事实(可利用)**:① 转移只依赖 T,任一代表都行;② 单 stack 每带唯一;③ 卷积权重 pass 本身很快(C=4 单态 0.11s);④ canonical 状态数极少。

**三条候选解法(按推荐度)**:
1. **代表作为权重 pass 的副产品(推荐)**:在算 weight 时,枚举的不是抽象 $(da,db)$ 而是具体的一对放置 $(p_T^X,p_B^X,p_T^Y,p_B^Y)$,顺手构造出具体 $(X',Y')$,对每个**首次出现**的目标 canonical key 存一个。O(1)/目标,不额外枚举。难点:要在不显著拖慢 weight pass 的前提下捕获(weight pass 用的是卷积 distA⊛distB,已抽象掉具体放置——需要一条"轻量放置"旁路只为取代表)。
2. **对齐枚举 + degree 剪枝**:$(2C)!$ 太大,但按符号的 (degX,degY) 度数桶剪枝(同 `src/wl_canon.cpp` 思路),只在桶内置换 → 大幅降。注意 WL 在高对称态会退化(见 §6),需配合;但"找一个代表"比"完全规范化"宽松,可能够用。
3. **预计算商转移矩阵**:既然转移只依赖 T 且状态少,把"canonical T → {canonical T': 权重}"在**所有可能 T 上**一次性算成固定矩阵,DP 纯查表。需要先枚举所有可达 canonical T(用 1 或 2 解决发现)。理论依据:Clarke 1993 商转移图(对称约简)。

**验证闸门(每步必跑)**:改完先 `./build/final2xC.exe 3 dump` 对 §3 金标准;通过后 `./build/final2xC.exe 4` 必须 = `29136487207403520`;再 C=5 = `1903816047972624930994913280000`;**都过了才信 C=6 的输出**。

---

## 6. 踩过的坑(教训,别重蹈)

1. **#P 内核反复出现**:外层枚举、单带 B(σ)、联合 placement —— 三处都撞 permanent/#P。正确路是"卷积转移 + 商状态",已找到。
2. **三次 bug 全被验证链挡下**(这是方法论核心,务必延续):
   - `band_count` 漏了 kjellfp 约束 (ii) → N 偏大(得 $2^{2C}$ 而非 $\binom{2C}{C}$)。**教训**:用两族独立闭式(R=2 & R=3 Franel)同时卡死才敢信。
   - WL canon 差分测试否决(2251/3000 不一致)→ WL 在高对称态退化。**教训**:任何 canon/规范化,先和慢全-canon 差分对拍几千例再用。
   - Δ "= 两置换矩阵"漏了交叉项 → 权重对但 T 值偏移。**教训**:非空带必须单独 brute,空状态对不代表非空对。
3. **数值溢出**:N(2×6) > u128。**必须 bignum**(已备)。别用 u128 跑 C≥6。
4. **`-march=native` 段错误**(MinGW AVX)。用 `-mpopcnt`,别用 native。
5. **别混淆"带数 $\sum B$"与"网格数 $\sum B^2$"**:`band_count.cpp` 是前者(kjellfp 一阶矩,多项式),网格是后者(二阶矩,需卷积转移)。
6. **代码 bug 会反污染对推导的判断**:每次都让已验证的 Python/oracle 当 C++ 的裁判,逐带对,不只对总数。

---

## 7. 验收标准(怎么算"成功")

- **最低**:C=4 跑出 `29136487207403520`(引擎在 C=4 闭合,证明方法+实现端到端正确)。
- **达标**:C=5 跑出 `1903816047972624930994913280000`(复现 OEIS 最后一项)。
- **突破**:C=6 跑出一个值。**提交 OEIS A291187 前**:用第二套独立方法或更高精度复算交叉验证(诚实:这是站在 Pettersen/kjellfp/Russell 方法上的独立实现,署名/优先权属原方法发明者,我们的贡献是独立验证 + 可能的新项计算)。

---

## 8. 一句话状态

**数学登顶、数值就绪、转移核验证;就差"高效代表发现"这一处纯实现优化,即可让 C=4→5→6 端到端打通。** 接力点 = `src/final2xC.cpp` + 本文件 §5。

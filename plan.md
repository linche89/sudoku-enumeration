
# SUDOKU ENUMERATION: FJ05 ALGORITHM REPRODUCTION SPECIFICATION

## 1. 架构目标与上下文 (Architecture Objective)

本工程目标为通过底层计算语言（推荐 C++17/20 或 Rust）重现 Bertram Felgenhauer 与 Frazer Jarvis 在 2005 年确立的 $9 \times 9$ 标准数独终局总数证明。算法核心并非纯暴力 DFS，而是**群论对称性降维（Symmetry Reduction）与底层位运算穷举（Bitwise Exhaustion）的结合**。Agent 需严格遵循下述管线设计，杜绝冗余对象分配与深拷贝。

## 2. 核心计算管线 (Computation Pipeline)

系统必须实现以下四个阶段的串联：

```text
+--------------------------------------------------------------------------------+
|                   SUDOKU ENUMERATION PIPELINE (FJ05 MODEL)                     |
+--------------------------------------------------------------------------------+
|  [PHASE 1: Canonicalization / B1 冻结]                                         |
|  将左上角 3x3 宫格 (B1) 强行设定为 1-9 标准序。                                |
|  数学影响：生成全局乘数因子 9! = 362,880。                                     |
+---------+----------------------------------------------------------------------+
          |
          v
+---------+----------------------------------------------------------------------+
|  [PHASE 2: Top Band Generation / 顶带生成]                                     |
|  生成 B2 与 B3 顶行组合（区分 Pure/Mixed 行排列），并在满足数独规则前提下扩充  |
|  至前三行。                                                                    |
|  空间规模：合法 Top Band 组合共计 2,612,736 种。                               |
+---------+----------------------------------------------------------------------+
          |
          v
+---------+----------------------------------------------------------------------+
|  [PHASE 3: Lexicographical & Isomorphic Reduction / 拓扑降维]                  |
|  1. 块内列互换与块互换 -> 压缩至 36,288 种代表态。                             |
|  2. 全局数字重映射 (Relabeling) & 2x2/2x3 子矩阵等价性识别。                   |
|  空间规模：坍缩至 71 个终极等价类 (Equivalence Classes)。                      |
+---------+----------------------------------------------------------------------+
          |
          v
+---------+----------------------------------------------------------------------+
|  [PHASE 4: Parallel Bitwise DFS / 并行位图回溯]                                |
|  输入：71 个等价类矩阵及其对应的轨道大小 (Orbit Size, 记为 S_i)。              |
|  执行：针对每个类底下剩余的 54 格执行超高频 DFS，输出解集数量 (C_i)。          |
+---------+----------------------------------------------------------------------+
          |
          v
+---------+----------------------------------------------------------------------+
|  [PHASE 5: Global Aggregation / 全局聚合]                                      |
|  N1 = Σ (C_i * S_i), 其中 i ∈ [1, 71]。理论值 N1 = 18,383,222,420,692,992。    |
|  N = N1 * 9! = 6,670,903,752,021,072,936,960。                                 |
+--------------------------------------------------------------------------------+

```

## 3. 核心算子设计：O(1) 位掩码验证 (Bitmask Validation)

为应对 PHASE 4 中百亿级别的节点遍历，Agent 严禁使用数组遍历或 `if-else` 分支进行数独有效性验证。必须在系统底层利用 16-bit 无符号整数及 CPU 内置硬件指令进行降维操作。

```text
+---------------------------------------------------------+
| BITWISE STATE MASKING & PRUNING                         |
+---------------------------------------------------------+
| Digit Availability (1-9) in u16:                        |
| Mask Bit Index: 0 b 0 0 0 0 0 0 0 0 0 0                 |
|                     ^ ^ ^ ^ ^ ^ ^ ^ ^                   |
| Digits:             9 8 7 6 5 4 3 2 1                   |
|                                                         |
| Row Mask [r]:     0b0010010010 (Digits 1, 4, 7 used)    |
| Col Mask [c]:     0b0100010001 (Digits 1, 5, 9 used)    |
| Box Mask [b]:     0b0001100010 (Digits 1, 6, 7 used)    |
|                                                         |
| 1. Bitwise OR (Used): Row | Col | Box                   |
|    Result: 0b0111110011 (Digits 1,4,5,6,7,9 used)       |
|                                                         |
| 2. Bitwise NOT & Filter (Available): (~Used) & 0x3FE    |
|    Result: 0b0000001100 (Digits 2, 3 available)         |
+---------------------------------------------------------+

```

### 3.1 DFS 循环展开伪代码规范

Agent 生成的 DFS 热点循环必须运用 `LSB (Least Significant Bit)` 隔离与清零技术。

* **隔离最低位的 1**: `bit = available & -available;` (或使用 Rust 的 `available & available.wrapping_neg()`)
* **推进迭代**: `available &= available - 1;`
* **硬件指令**: 提取实际数字使用 `__builtin_ctz` (C++) 或 `trailing_zeros()` (Rust)。

## 4. 并发与内存模型 (Concurrency Model)

要求采用 Work-Stealing 队列或静态分发机制，彻底榨干现代多核处理器的算力。内存布局应确保每个 Worker 独占一条 Cache Line 以避免伪共享 (False Sharing)。

```text
+-----------------------------------------------------------------+
| PARALLEL EXECUTION TOPOLOGY                                     |
+-----------------------------------------------------------------+
| Master Thread: Dispatch 71 Equivalence Classes (EC)             |
+--------------------------+--------------------------------------+
                           | Atomic Work Queue
      +--------------------+--------------------+
      |                    |                    |
+-----v-----+        +-----v-----+        +-----v-----+
| Worker 1  |        | Worker 2  |        | Worker N  |
| Task: EC_5|        | Task: EC_12|       | Task: EC_44|
| S_i = ... |        | S_i = ...  |       | S_i = ... |
+-----------+        +-----------+        +-----------+
| State Init|        | State Init |       | State Init|
| L1 Cache  |        | L1 Cache   |       | L1 Cache  |
+-----------+        +-----------+        +-----------+
| CTZ DFS   |        | CTZ DFS    |       | CTZ DFS   |
+-----+-----+        +-----+-----+        +-----+-----+
      |                    |                    |
      +--------------------+--------------------+
                           | Atomic Accumulator
+--------------------------v--------------------------------------+
| GLOBAL STATE: N1.fetch_add( C_i * S_i, Ordering::SeqCst )       |
+-----------------------------------------------------------------+

```

## 5. Agent 代码生成验收标准 (Verification Checkpoints)

1. **静态类型隔离**：DFS 核心引擎中不得出现任何堆分配 (`std::vector` / `Vec`)。矩阵状态需硬编码为一维扁平化数组 `[u16; 9]` 用于行列校验。
2. **71 类等价数据源**：程序应提供模块化的接口以载入已计算好的 71 个代表性 Top Band 及其对应 Orbit Size。此部分数据可参照 FJ05 原始 C++ 文件 `sudoku_equiv.cc` 的输出结果。
3. **大数溢出处理**：聚合变量必须使用 `uint64_t` (C++) 或 `u64` (Rust) 以承载 18,383,222,420,692,992 这个中继数值；最终乘法必须使用 `unsigned __int128` 或 `u128` 以正确输出 $6.67 \times 10^{21}$ 的 22 位精确度，杜绝浮点数截断。

## 6. 文献基准 (References)

* **核心逻辑出处**: *Mathematics of Sudoku I* (Bertram Felgenhauer, Frazer Jarvis, 2006). [🔗 http://www.afjarvis.org.uk/maths/felgenhauer_jarvis_spec1.pdf](http://www.afjarvis.org.uk/maths/felgenhauer_jarvis_spec1.pdf)
* **代码参考映射**: 原始计算流水线依赖 Haskell (生成初始集) + C++ (同构压缩) + C++ (DFS 验证)。本次复现要求利用现代语言特性整合阶段 4 与阶段 5。 [🔗 http://www.afjarvis.org.uk/sudoku/bertram.html](http://www.afjarvis.org.uk/sudoku/bertram.html)
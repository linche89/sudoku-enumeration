"""Independent finite order-6 reduced Latin square enumeration.

No Sudoku catalogue, production state, imported counting code, or network input.
Run only with the separate 30-second / 1-GiB process guard.
"""
from hashlib import sha256
from math import comb, factorial
import time

n = 6
full = (1 << n) - 1
board = [[-1] * n for _ in range(n)]
for i in range(n):
    board[0][i] = i
    board[i][0] = i
columns = [full] + [1 << i for i in range(1, n)]
started = time.monotonic()
visits = leaves = 0
digest = sha256()


def visit(row, col, used):
    global visits, leaves
    visits += 1
    if visits % 1024 == 0 and time.monotonic() - started > 25:
        raise RuntimeError('25-second cooperative bound; partial count is invalid')
    if row == n:
        assert all(set(line) == set(range(n)) for line in board)
        assert all({board[i][j] for i in range(n)} == set(range(n)) for j in range(n))
        assert board[0] == list(range(n))
        assert [board[i][0] for i in range(n)] == list(range(n))
        digest.update(bytes(value for line in board for value in line))
        leaves += 1
        return
    if col == n:
        visit(row + 1, 1, 1 << (row + 1))
        return
    choices = full & ~(columns[col] | used)
    while choices:
        bit = choices & -choices
        choices ^= bit
        board[row][col] = bit.bit_length() - 1
        columns[col] |= bit
        visit(row, col + 1, used | bit)
        columns[col] ^= bit
        board[row][col] = -1


visit(1, 1, 1 << 1)
assert leaves == 9408
multiplier = factorial(n) * factorial(n - 1)
latin = leaves * multiplier
print('REDUCED_LATIN_ORDER6', leaves)
print('LABELLED_MULTIPLIER', multiplier, '= 6! * 5!')
print('LABELLED_LATIN_ORDER6', latin)
print('REDUCED_ENUMERATION_ORDERED_SHA256', digest.hexdigest().upper())
for t in range(n // 2 + 1):
    words = sorted([0] * (n - t) + [63] * (n - t) + [1] * t + [62] * t)
    masks = sorted(sum(1 << (2 * b + ((word >> b) & 1)) for b in range(n)) for word in words)
    value, remainder = divmod(latin ** 2, comb(n, t))
    assert remainder == 0
    m = (1 << (n - 1)) if t == 0 else n * (1 << (n - 2 if 2 * t == n else n - 1))
    ell = factorial(2 * n) // (factorial(n - t) ** 2 * factorial(t) ** 2)
    print('C6_FAMILY t=%d m=%d ell=%d F=%d words=%s slot_masks=%s' % (t, m, ell, value, words, masks))
print('PASS visits=%d leaves=%d seconds=%.6f no_production_input=1' % (visits, leaves, time.monotonic() - started))

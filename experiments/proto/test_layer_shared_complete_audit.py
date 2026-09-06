"""Finite parser/counter refusal tests; no production checkpoint access."""
import copy
from pathlib import Path
import unittest
from unittest.mock import patch

from layer_shared_complete_audit import controller_check, parse_log, reconcile


class CompleteAuditTests(unittest.TestCase):
    def setUp(self):
        self.rows = [dict(begin=0, count=25000, live=25000, representatives=1, value_checksum=24),
                     dict(begin=25000, count=25000, live=24982, representatives=2, value_checksum=120)]
        self.totals = dict(closed_prefix=50000, live_records=49982, holes=18,
            prior_live_records=25000, prior_closed_representatives=1, prior_value_checksum=24,
            closed_representatives=3, value_checksum=144, aliases_to_closed_representatives=49982,
            aliases_to_future_uncomputed_representatives=0)
        self.raw = (
            'RESUMED chunks=1 closed_prefix=25000 closed_representatives=1\n'
            'CLOSED_CHUNK begin=25000 count=25000 live=24982 representatives=2 closed_prefix=50000/50000\n'
            'SUMMARY status=CLOSED_F4_CATALOGUE domain=complete_native_L4 new_chunks=1 new_indices=25000 '
            'closed_prefix=50000 total_entries=50000 closed_representatives=3 F4_checksum_mod2_64=144 N6=NOT_COMPUTED\n'
        ).encode('ascii')
        self.log = parse_log(self.raw)

    def run_reconcile(self, rows=None, totals=None, log=None):
        with patch('layer_shared_complete_audit.N', 50000):
            reconcile(self.rows if rows is None else rows, self.totals if totals is None else totals,
                      self.log if log is None else log)

    def test_valid_counter_reconciliation(self):
        self.run_reconcile()

    def test_reject_modified_decoded_counters(self):
        for key in self.totals:
            with self.subTest(key=key):
                bad = dict(self.totals)
                bad[key] += 1
                with self.assertRaises(ValueError):
                    self.run_reconcile(totals=bad)

    def test_reject_modified_resume_and_terminal(self):
        for part in (0, 2):
            for key, value in self.log[part].items():
                with self.subTest(part=part, key=key):
                    bad = copy.deepcopy(self.log)
                    bad[part][key] = str(int(value)+1) if value.isdigit() else 'INVALID'
                    with self.assertRaises(ValueError):
                        self.run_reconcile(log=bad)

    def test_reject_modified_chunk_and_inventory(self):
        for key, value in self.log[1][0].items():
            with self.subTest(key=key):
                bad = copy.deepcopy(self.log)
                bad[1][0][key] = str(int(value)+1) if value.isdigit() else 'INVALID'
                with self.assertRaises(ValueError):
                    self.run_reconcile(log=bad)
        with self.assertRaises(ValueError):
            self.run_reconcile(log=(self.log[0], [], self.log[2]))

    def test_missing_or_duplicate_terminal(self):
        for raw in (b'', self.raw+self.raw, self.raw.split(b'SUMMARY')[0]):
            with self.assertRaises(ValueError):
                parse_log(raw)

    def test_controller_binding_encodings_and_refusals(self):
        engine = Path('E:/finite-fixture/logs')
        before, after = Path('D:/finite-fixture/before'), Path('D:/finite-fixture/after')
        text = (f'PHYSICAL_BACKUP phase=before files=2 directory={before}\r\n'
                f'PHYSICAL_BACKUP phase=after files=3 directory={after}\r\n'
                f'WINDOW_END exit=0 logs={engine}\r\n')
        for encoding in ('utf-8', 'utf-8-sig', 'utf-16'):
            controller_check(text.encode(encoding), engine, before, after, 2, 3)
        for bad in (text.replace('exit=0', 'exit=98'), text.replace('files=3', 'files=4'),
                    text.replace(str(engine), 'E:/wrong/logs'), text.splitlines()[-1], text+text):
            with self.assertRaises(ValueError):
                controller_check(bad.encode('utf-8'), engine, before, after, 2, 3)


if __name__ == '__main__':
    unittest.main(verbosity=2)

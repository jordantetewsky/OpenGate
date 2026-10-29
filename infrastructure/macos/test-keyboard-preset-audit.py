#!/usr/bin/env python3
import importlib.util,struct,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('audit',Path(__file__).with_name('audit-keyboard-preset.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def u(n):return struct.pack('>I',n)
def blob(b):return u(len(b))+b
def text(s):return blob(s.encode('utf-16-be'))
def inner(records,trailer=True):
 b=u(1)+u(len(records))
 for label,sequences in records:
  b+=text(label)+u(len(sequences))
  for sequence in sequences:b+=u(len(sequence))+b''.join(u(k) for k in sequence)
 return b+(text('DaVinci Resolve') if trailer else b'')
def outer(names):return u(1)+u(len(names))+b''.join(text(name)+blob(data) for name,data in names)
class AuditTests(unittest.TestCase):
 def test_alternates_and_sequence(self):
  records=[('OpenGate - Edit - Stabilize',[(0x0a000053,),(0x04000053,),(0x0400004b,0x04000053,0,0)])]
  actual,base=m.parse_inner(inner(records));self.assertEqual(actual[records[0][0]],[tuple(s for s in sequence if s) for sequence in records[0][1]]);self.assertEqual(base,'DaVinci Resolve')
 def test_empty_sentinel_and_legacy_trailer(self):
  records,base=m.parse_inner(inner([('Native',[(0,)])],False));self.assertEqual(records,{'Native':[]});self.assertIsNone(base)
 def test_missing_preset_has_no_bindings(self):
  presets=m.parse_outer(outer([('Other',inner([]))]));self.assertNotIn('Active',presets)
 def test_truncation(self):
  data=outer([('Active',inner([('Command',[(83,)])]))])
  for length in range(len(data)):
   with self.assertRaises((m.InvalidPreset,UnicodeError)):m.parse_outer(data[:length])
 def test_unknown_versions_and_trailing_data(self):
  for data in [u(2)+u(0),outer([])+b'x',u(1)+u(999999)]:
   with self.assertRaises(m.InvalidPreset):m.parse_outer(data)
 def test_duplicate_identity(self):
  with self.assertRaises(m.InvalidPreset):m.parse_outer(outer([('A',inner([])),('A',inner([]))]))
  with self.assertRaises(m.InvalidPreset):m.parse_inner(inner([('A',[]),('A',[])]))
 def test_sparse_sequence_rejected(self):
  with self.assertRaises(m.InvalidPreset):m.parse_inner(inner([('A',[(83,0,84,0)])]))
if __name__=='__main__':unittest.main()

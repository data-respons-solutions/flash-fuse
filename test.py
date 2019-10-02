#!/usr/bin/python3

import unittest
import tempfile
import os
import subprocess

def create_otp(dir, name, value):
    with open(os.path.join(dir, name), 'w') as f:
        f.write(f'{value}\n')
    
def get_otp(dir, name):
    with open(os.path.join(dir, name), 'r') as f:
        return '0x' + f.read().strip('\n')[2:].zfill(8)
        
def flash_fuse(dir, arglist):
    args = ['./flash-fuse-imx.py', '--fuse-path' , dir]
    args.extend(arglist)
    return subprocess.run(args).returncode

class test_CFG5_SJC_DISABLE(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
    def tearDown(self):
        self.tmpdir.cleanup()
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG5'
        cls.unfused = '0x00000000'
        cls.fused = '0x00100000'
        cls.arg = ['--fuse', 'CFG5_SJC_DISABLE']
        
    def test_burn(self):
        create_otp(self.dir, self.ocotp, self.unfused)
        self.assertEqual(0, flash_fuse(self.dir, self.arg + ['--commit']))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.fused)
        
    def test_burn_already_fused(self):
        create_otp(self.dir, self.ocotp, self.fused)
        self.assertEqual(0, flash_fuse(self.dir, self.arg + ['--commit']))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.fused)
                
    def test_verify_not_fused(self):
        create_otp(self.dir, self.ocotp, self.unfused)
        self.assertEqual(1, flash_fuse(self.dir, self.arg))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.unfused)
            
    def test_verify_fused(self):
        create_otp(self.dir, self.ocotp, self.fused)
        self.assertEqual(0, flash_fuse(self.dir, self.arg))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.fused)
            
class test_CFG5_DIR_BT_DIS(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG5'
        cls.unfused = '0x00000000'
        cls.fused = '0x00000008'
        cls.arg = ['--fuse', 'CFG5_DIR_BT_DIS']
        
class test_CFG5_BT_FUSE_SEL(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG5'
        cls.unfused = '0x00000000'
        cls.fused = '0x00000010'
        cls.arg = ['--fuse', 'CFG5_BT_FUSE_SEL']

class test_mac(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
    def tearDown(self):
        self.tmpdir.cleanup()

    def test_burn(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
              
    def test_burn_already_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
                
    def test_burn_already_fused_wrong_mac(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0xff334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0xff334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

    def test_verify_not_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000000')
            
    def test_verify_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

if __name__ == '__main__':
    unittest.main()
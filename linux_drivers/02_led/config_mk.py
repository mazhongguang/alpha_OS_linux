import os
import sys

# ycm_flag
ycm_flags = [
        'linux',
        '/home/user/linux/IMX6ULL/linux/linux_nxp_mine/include',
        '/home/user/linux/IMX6ULL/linux/linux_nxp_mine/arch/arm/include',
        '/home/user/linux/IMX6ULL/linux/linux_nxp_mine/arch/arm/include/generated',
        ]
# 生成YCM配置文件需要添加的头文件列表
with open('ycm_flag.py','w') as ycm_flag_file :
    ycm_flag_file.write("#以下是YCM配置文件需要的FLAG内容\n")
    ycm_flag_file.write("flag = [ \n")
    # for a in ycm_flag_c_define :
        # ycm_flag_file.write("'" + a + "',\n")
    for a in ycm_flags :
        ycm_flag_file.write("'-I',\n'" + a + "',\n")
    ycm_flag_file.write("]\n")


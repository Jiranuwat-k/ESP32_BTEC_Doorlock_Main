Import("env")

# 1. ดึงค่า Version ที่เราตั้งไว้ใน platformio.ini
my_version = env.GetProjectOption("custom_app_version")
my_name = env.GetProjectOption("custom_app_name")

# 2. ตั้งชื่อไฟล์ .bin ใหม่ (เช่น firmware_smartfarm_v1.0.1.bin)
env.Replace(PROGNAME="Firmware_%s_v%s" % (my_name, my_version))

# 3. ส่งค่า Version เข้าไปใน Code C++ (สร้าง Macro ชื่อ APP_VERSION และ APP_NAME)
env.Append(CPPDEFINES=[
    ("APP_VERSION", "\\\"" + my_version + "\\\""),
    ("APP_NAME", "\\\"" + my_name + "\\\""),
    ("FIRMWARE_FILENAME", "\\\"Firmware_%s_v%s.bin\\\"" % (my_name, my_version))
])
-- 以附加的方式打开只写文件
local file = io.open("/data/data/com.xt.xt.xt/files/InjectLua.log", "a")
-- 在文件最后一行添加 Lua 注释
file:write("Write" .. "\n")
-- 关闭打开的文件
file:close()

-- local filePath = FilePath .. "/" .. fileName .. ".txt"
-- local fileWriter = io.open(filePath, "w+")
-- fileWriter:write(table.concat(content))
-- fileWriter:close()
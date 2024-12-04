local oldclass = 0
local function newclass(moduleName, ...)
  local classRet = oldclass(moduleName, ...)
  myFilePrint("newclass hook moduleName : " .. tostring(moduleName) .. " classRet-type:" .. type(classRet))
  return classRet
end
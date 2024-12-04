local function myPrint(msg)
    segmentSize = 1000;
    msgLen = string.len(msg)
    MyLog("myPrint msgLen:" .. tostring(msgLen))
    if(msgLen <= segmentSize) then
        MyLog(msg)
    else
        loopCount = msgLen / segmentSize
        for i = 0, loopCount, 1 do
            stringIndexStart = i * segmentSize
            stringIndexEnd = (i+1) * segmentSize
            msgContent = string.sub(msg, stringIndexStart, stringIndexEnd)
            MyLog(msgContent)
        end
    end
end

local function myTableToString (t)
    local print_r_cache={}
    local function sub_print_r(t,indent)
        if (print_r_cache[tostring(t)]) then
            MyLog(indent.."*"..tostring(t))
        else
            print_r_cache[tostring(t)]=true
            if (type(t)=="table") then
                for pos,val in pairs(t) do
                    if (type(val)=="table") then
                        MyLog(indent.."["..pos.."] => "..tostring(t).." {")
                        sub_print_r(val,indent..string.rep(" ",string.len(pos)+8))
                        MyLog(indent..string.rep(" ",string.len(pos)+6).."}")
                    elseif (type(val)=="string") then
                        MyLog(indent.."["..pos..'] => "'..val..'"')
                    else
                        MyLog(indent.."["..pos.."] => "..tostring(val))
                    end
                end
            else
                MyLog(indent..tostring(t))
            end
        end
    end
    if (type(t)=="table") then
        MyLog(tostring(t).." {")
        sub_print_r(t,"  ")
        MyLog("}")
    else
        sub_print_r(t,"  ")
    end
end

local function traceBack()
    MyLog("***************************************************************************************")
    MyLog(debug.traceback())
    MyLog("***************************************************************************************")
end

local old_require = require
local function new_require(moduleName, ...)
    local moduleAddr = old_require(moduleName, ...)
    MyLog("new_require hook moduleName : " .. tostring(moduleName) .. " type:" .. type(moduleAddr))
    return moduleAddr
end

require = new_require
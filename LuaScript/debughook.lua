local function traceBack()
    MyLog("***************************************************************************************")
    MyLog(debug.traceback())
    MyLog("***************************************************************************************")
end

local function TableToStringAndHack(mTable, tag, hackKey)
    MyLog("*******************************************************************************")
    local print_r_cache = {}

    local function sub_print_r(mTable, indent)
        if (print_r_cache[tostring(mTable)]) then
            MyLog(indent .. "*" .. tostring(mTable))
        else
            print_r_cache[tostring(mTable)] = true
            if (type(mTable) == "table") then
                for pos, val in pairs(mTable) do
                    if (val ~= nil and type(val) == "table") then
                        if(val.m_stAbility ~= nil) then
                            MyLog("find it!!!!!!!!!!!!!!!!!!!!!")
                        end
                    end
                    if (type(val) == "table") then
                        MyLog(indent .. "table [" .. pos .. "] => " .. tostring(mTable) .. " {")
                        sub_print_r(val, indent .. string.rep(" ", string.len(pos) + 8))
                        MyLog(indent .. string.rep(" ", string.len(pos) + 6) .. "}")
                    elseif (type(val) == "string") then
                        MyLog(indent .. "string [" .. pos .. '] => "' .. val .. '"')
                    else
                        MyLog(indent .. "else [" .. pos .. "] => " .. tostring(val))
                    end
                end
            else
                MyLog(indent .. tostring(mTable))
            end
        end
    end

    if (type(mTable) == "table") then
        MyLog(tostring(mTable) .. " {")
        sub_print_r(mTable, "  ")
        MyLog("}")
    else
        sub_print_r(mTable, "  ")
    end
    MyLog("*******************************************************************************")
end


local function TableToString(mTable, Tag)
    MyLog("*******************************************************************************")
    local print_r_cache = {}
    local function sub_print_r(mTable, indent)
        if (print_r_cache[tostring(mTable)]) then
            MyLog(indent .. "*" .. tostring(mTable))
        else
            print_r_cache[tostring(mTable)] = true
            if (type(mTable) == "table") then
                for pos, val in pairs(mTable) do
                    if (type(val) == "table") then
                        MyLog(indent .. "[" .. pos .. "] => " .. tostring(mTable) .. " {")
                        sub_print_r(val, indent .. string.rep(" ", string.len(pos) + 8))
                        MyLog(indent .. string.rep(" ", string.len(pos) + 6) .. "}")
                    elseif (type(val) == "string") then
                        MyLog(indent .. "[" .. pos .. '] => "' .. val .. '"')
                    else
                        MyLog(indent .. "[" .. pos .. "] => " .. tostring(val))
                    end
                end
            else
                MyLog(indent .. tostring(mTable))
            end
        end
    end

    if (type(mTable) == "table") then
        MyLog(tostring(mTable) .. " {")
        sub_print_r(mTable, "  ")
        MyLog("}")
    else
        sub_print_r(mTable, "  ")
    end
    MyLog("*******************************************************************************")
end

local function tracer(info, sourceName, isPrintArg)
    local outBuffer = ""
    if(sourceName ~= nil) then
        if (string.find(info.source, sourceName) ~= nil) then
            outBuffer = outBuffer .. "source:" .. "[" .. tostring(info.source) .. "] name:[" .. tostring(info.name) .. "]"
            MyLog(outBuffer)
        end
    else
        outBuffer = outBuffer .. "source:" .. "[" .. tostring(info.source) .. "] name:[" .. tostring(info.name) .. "]"
        MyLog(outBuffer)
    end
    
    -- if (isPrintArg == true) then
    --     local a = 1
    --     while true do
    --         local key, value = debug.getlocal(4, a)
    --         if (not key or not value) then
    --             break
    --         end
    --         outBuffer = outBuffer .. " value:" .. tostring(value)
    --         a = a + 1
    --     end
    -- end
    
end

local function hookCB(event, ...)
    local info = debug.getinfo(3)
    if (not info) then
        return
    end
    if (not info.source or not info.name) then
        return
    end
    tracer(info, nil, false)
    if (event == "call") then
        -- local info = debug.getinfo(3)
        -- if (not info) then
        --     return
        -- end
        -- if (not info.source or not info.name) then
        --     return
        -- end
        -- tracer(info, nil, false)
        -- local fd = io.open('/data/local/tmp/log', "r")
        -- if fd ~= nil then
        --     MyLog("hello fd:" .. tostring(fd))
        --     local info = debug.getinfo(3)
        --     if (not info) then
        --         return
        --     end
        --     if (not info.source or not info.name) then
        --         return
        --     end
        --     tracer(info, nil, false)
        --     fd.close()
        -- end
        --tracer(info, nil, false)
        --tracer(info, "LuaBossBattleMgr", false)
        --tracer(info, "UIBattleView", false)
        --tracer(info, "LuaGameMgr", false)
    end
end

MyLog("enter debughookTest 6666")
debug.sethook(hookCB, "clr")
MyLog("leave debughookTest 222")
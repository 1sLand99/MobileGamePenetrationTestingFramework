local function myFilePrint(msg, filePath)
    local file = io.open(filePath, "a")
    file:write(msg .. "\n")
    file:close()
end

local function myFileTableToString(t, filePath)
    local print_r_cache = {}
    local function sub_print_r(t, indent)
        if (print_r_cache[tostring(t)]) then
            myFilePrint(indent .. "*" .. tostring(t), filePath)
        else
            print_r_cache[tostring(t)] = true
            if (type(t) == "table") then
                for pos, val in pairs(t) do
                    if (type(val) == "table") then
                        myFilePrint(indent .. "[" .. pos .. "] => " .. tostring(t) .. " {", filePath)
                        sub_print_r(val, indent .. string.rep(" ", string.len(pos) + 8))
                        myFilePrint(indent .. string.rep(" ", string.len(pos) + 6) .. "}", filePath)
                    elseif (type(val) == "string") then
                        myFilePrint(indent .. "[" .. pos .. '] => "' .. val .. '"', filePath)
                    else
                        myFilePrint(indent .. "[" .. pos .. "] => " .. tostring(val), filePath)
                    end
                end
            else
                myFilePrint(indent .. tostring(t), filePath)
            end
        end
    end
    if (type(t) == "table") then
        myFilePrint(tostring(t) .. " {", filePath)
        sub_print_r(t, "  ")
        myFilePrint("}", filePath)
    else
        sub_print_r(t, "  ")
    end
end

local function myPrint(msg)
    local segmentSize = 1000
    local msgLen = string.len(msg)
    -- MyLog("myPrint msgLen:" .. tostring(msgLen))
    if msgLen <= segmentSize then
        MyLog(msg)
    else
        local loopCount = math.floor(msgLen / segmentSize)
        for i = 0, loopCount do
            local stringIndexStart = i * segmentSize + 1  -- Lua的字符串索引从1开始
            local stringIndexEnd = math.min((i + 1) * segmentSize, msgLen)
            local msgContent = string.sub(msg, stringIndexStart, stringIndexEnd)
            MyLog(msgContent)
        end
    end
end

local function myPrintTableToString(t)
    local print_r_cache = {}
    local function sub_print_r(t, indent)
        if print_r_cache[tostring(t)] then
            myPrint(indent .. "*" .. tostring(t))
        else
            print_r_cache[tostring(t)] = true
            if type(t) == "table" then
                for pos, val in pairs(t) do
                    local posStr = tostring(pos)  -- 确保 pos 是字符串
                    local posLen = string.len(posStr)
                    if type(val) == "table" then
                        myPrint(indent .. "[" .. posStr .. "] => " .. tostring(t) .. " {")
                        sub_print_r(val, indent .. string.rep(" ", posLen + 8))
                        myPrint(indent .. string.rep(" ", posLen + 6) .. "}")
                    elseif type(val) == "string" then
                        myPrint(indent .. "[" .. posStr .. '] => "' .. val .. '"')
                    else
                        myPrint(indent .. "[" .. posStr .. "] => " .. tostring(val))
                    end
                end
            else
                myPrint(indent .. tostring(t))
            end
        end
    end
    if type(t) == "table" then
        myPrint(tostring(t) .. " {")
        sub_print_r(t, "  ")
        myPrint("}")
    else
        sub_print_r(t, "  ")
    end
end
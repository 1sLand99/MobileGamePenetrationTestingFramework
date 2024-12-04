local clock = os.clock

local function sleep(n) -- seconds
    local t0 = clock()
    while clock() - t0 <= n do end
end

local function studythread()
    local co = coroutine.create(
        function()
            while true do
                MyLog("Hello I am alive")
                -- os.execute("sleep " .. 1)  这个和下面的方式，都是整个虚拟机卡住的。
                -- sleep(1)
            end
        end
    )
    coroutine.resume(co, 1) -- 1
    MyLog(coroutine.status(co)) -- dead
end

studythread()
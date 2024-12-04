setmetatable(_G, {
    __newindex = function(table, key, value)
        rawset(table, key, value)
    end
})
local function hookAllMethods(classInstance, hookFunction)
    for methodName, method in pairs(classInstance) do
        if type(method) == "function" then
            classInstance[methodName] = function(...)
                return hookFunction(classInstance, methodName, method, ...)
            end
        end
    end
end

local function myHookFunction(instance, methodName, originalFunction, ...)
    myPrint("Instance: " .. tostring(instance) .. "Function " .. methodName .. " is called")
    return originalFunction(...)
end

hookAllMethods(sceneProxyInstance, myHookFunction)

local sceneProxyInstance = require("SceneProxy")
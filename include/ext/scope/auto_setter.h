#pragma once

#include <functional>
#include <optional>
#include <type_traits>

namespace ext::scope {

// Auto set variable values inside scope
template <class Type, class VariableType>
class AutoSet
{
public:
    // Set initial value for variable and value on exit scope.
    AutoSet(Type& var, VariableType initialValue, VariableType onExitValue)
        : m_var(var)
        , m_onExitValue(std::move(onExitValue))
    {
        m_var = std::move(initialValue);
    }

    // Set value to variable on exit scope
    AutoSet(Type& var, VariableType onExitValue)
        : m_var(var)
        , m_onExitValue(std::move(onExitValue))
    {}

    ~AutoSet()
    {
        m_var = m_onExitValue;
    }

private:
    Type& m_var;
    VariableType m_onExitValue;
};

/// AutoSet only for complex objects - for example, to save state in a class
/// where the current state is taken via getState and set via setState
template <class Type>
class AutoSetFunc
{
public:
    /// <summary>Constructor to store the current value in a function..</summary>
    /// <param name="getValueFunc">The function used to get the current value</param>
    /// <param name="setValueFunc">The function that will be called to set the value</param>
    /// <param name="needExecuteWork">Flag checking the need to perform any action(result of check function)</param>
    AutoSetFunc(const std::function<Type(void)>& getValueFunc,
                const std::function<void(const Type&)>& setValueFunc,
                bool needExecuteWork = true)
    {
        if (needExecuteWork)
        {
            m_savedInitialValue = getValueFunc();
            m_setValueFunc = setValueFunc;
        }
    }

    /// <summary>Constructor using a function to get an initial value for setting into a function</summary>
    /// <param name="getValueFunc">The function used to get the current value</param>
    /// <param name="setValueFunc">The function to be called to set the stored value</param>
    /// <param name="newValue">The new value to be set to the function</param>
    /// <param name="needExecuteWork">Flag checking the need to perform any action(result of check function)</param>
    AutoSetFunc(const std::function<Type(void)>& getValueFunc,
                const std::function<void(const Type&)>& setValueFunc,
                const Type& newValue,
                bool needExecuteWork = true)
    {
        if (needExecuteWork)
        {
            m_savedInitialValue = getValueFunc();

            m_setValueFunc = setValueFunc;
            m_setValueFunc(newValue);
        }
    }

    /// <summary>Constructor with the current value</summary>
    /// <param name="currentValue">Current value</param>
    /// <param name="setValueFunc">The function that will be called to restore the saved value</param>
    /// <param name="needExecuteWork">Flag checking the need to perform any action(result of check function)</param>
    AutoSetFunc(const Type& currentValue,
                const std::function<void(const Type&)>& setValueFunc,
                bool needExecuteWork = true)
    {
        if (needExecuteWork)
        {
            m_savedInitialValue = currentValue;
            m_setValueFunc = setValueFunc;
        }
    }

    ~AutoSetFunc()
    {
        if (m_savedInitialValue.has_value())
            m_setValueFunc(*m_savedInitialValue);
    }

private:
    std::optional<Type> m_savedInitialValue;
    std::function<void(const Type&)> m_setValueFunc;
};

} // namespace ext::scope
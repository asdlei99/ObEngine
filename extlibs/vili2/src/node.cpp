#include <iostream>
#include <iterator>
#include <vili2/node.hpp>

namespace vili
{
    node_iterator::node_iterator(array::value_type* value)
        : m_ptr(value)
    {
    }

    node_iterator::node_iterator(object::value_type* value)
        : m_ptr(value)
    {
    }

    node_iterator::node_iterator(const node_iterator& other_it)
        : m_ptr(other_it.m_ptr)
    {
    }

    node_iterator& node_iterator::operator++()
    {
        if (std::holds_alternative<array::value_type*>(m_ptr))
        {
            ++std::get<array::value_type*>(m_ptr);
        }
        if (std::holds_alternative<object::value_type*>(m_ptr))
        {
            ++std::get<object::value_type*>(m_ptr);
        }

        return *this;
    }

    bool node_iterator::operator!=(const node_iterator& rhs) const
    {
        if (std::holds_alternative<array::value_type*>(m_ptr))
        {
            return std::get<array::value_type*>(m_ptr)
                != std::get<array::value_type*>(rhs.m_ptr);
        }
        if (std::holds_alternative<object::value_type*>(m_ptr))
        {
            return std::get<object::value_type*>(m_ptr)
                != std::get<object::value_type*>(rhs.m_ptr);
        }
    }

    node& node_iterator::operator*()
    {
        if (std::holds_alternative<array::value_type*>(m_ptr))
        {
            return *std::get<array::value_type*>(m_ptr);
        }
        if (std::holds_alternative<object::value_type*>(m_ptr))
        {
            return std::get<object::value_type*>(m_ptr)->second;
        }
    }

    std::string node::dump_array() const
    {
        const auto& vector = std::get<array>(m_data);
        std::string dump_value = "[";
        for (auto it = vector.begin(); it != vector.end(); ++it)
        {
            dump_value += it->dump() + (it != (vector.end() - 1) ? ", " : "");
        }
        dump_value += "]";
        return dump_value;
    }

    std::string node::dump_object(bool root) const
    {
        const auto& map = std::get<object>(m_data);
        std::string dump_value = (root) ? "" : "{";
        for (const auto& [key, value] : map)
        {
            if (!value.is_null())
                dump_value += key + ": " + value.dump() + ", ";
        }
        if (!root)
            dump_value += "}";
        return dump_value;
    }

    node::node(int value)
    {
        m_data = static_cast<integer>(value);
    }

    node::node(integer value)
    {
        m_data = value;
    }

    node::node(number value)
    {
        m_data = value;
    }

    node::node(const string& value)
    {
        m_data = value;
    }

    node::node(boolean value)
    {
        m_data = value;
    }

    node::node(const char* value)
    {
        m_data = std::string(value);
    }

    node::node(const array& value)
    {
        m_data = value;
    }

    node::node(const object& value)
    {
        m_data = value;
    }

    node::node(const node& copy)
    {
        m_data = copy.m_data;
    }

    node::node(node&& move) noexcept
    {
        m_data = std::move(move.m_data);
    }

    void node::operator=(const node& copy)
    {
        m_data = copy.m_data;
    }

    node_type node::type() const
    {
        if (is_null())
            return node_type::null;
        if (std::holds_alternative<integer>(m_data))
            return node_type::integer;
        if (std::holds_alternative<number>(m_data))
            return node_type::number;
        if (std::holds_alternative<boolean>(m_data))
            return node_type::boolean;
        if (std::holds_alternative<string>(m_data))
            return node_type::string;
        if (std::holds_alternative<object>(m_data))
            return node_type::object;
        if (std::holds_alternative<array>(m_data))
            return node_type::array;
    }

    std::string node::dump(bool root) const
    {
        if (is_null())
            return "";
        if (is<number>())
            return utils::string::truncate_float(std::to_string(as<number>()));
        if (is<integer>())
            return std::to_string(as<integer>());
        if (is<string>())
            return utils::string::quote(as<string>());
        if (is<boolean>())
            return (as<boolean>() ? "true" : "false");
        if (is<array>())
            return dump_array();
        if (is<object>())
            return dump_object(root);
    }

    bool node::is_primitive() const
    {
        if (is<integer>() || is<number>() || is<string>() || is<boolean>())
            return true;
        return false;
    }

    bool node::is_container() const
    {
        if (is<array>() || is<object>())
            return true;
        return false;
    }

    bool node::is_null() const
    {
        if (m_data.index())
            return false;
        return true;
    }

    node& node::operator[](const char* key)
    {
        return operator[](std::string(key));
    }

    node& node::operator[](const std::string& key)
    {
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            return map[key];
        }
        throw exceptions::invalid_cast(object_type, to_string(type()), VILI_EXC_INFO);
    }

    node& node::operator[](const size_t index)
    {
        return this->at(index);
    }

    size_t node::size() const
    {
        if (is<array>())
        {
            return std::get<array>(m_data).size();
        }
        if (is<object>())
        {
            return std::get<object>(m_data).size();
        }
        throw exceptions::invalid_cast(object_type, to_string(type()), VILI_EXC_INFO);
    }

    bool node::empty() const
    {
        return this->size() == 0;
    }

    void node::clear()
    {
        if (is<array>())
        {
            std::get<array>(m_data).clear();
        }
        else if (is<object>())
        {
            std::get<object>(m_data).clear();
        }
        else
        {
            throw exceptions::invalid_cast(
                object_type, to_string(type()), VILI_EXC_INFO); // TODO: Add array type
        }
    }

    node::operator std::string_view() const
    {
        return as<string>();
    }

    node::operator const std::basic_string<char>&() const
    {
        return as<string>();
    }

    node::operator integer() const
    {
        return as<integer>();
    }

    node::operator number() const
    {
        return as<number>();
    }

    node::operator boolean() const
    {
        return as<boolean>();
    }

    node::operator unsigned() const
    {
        return as<integer>();
    }

    std::ostream& operator<<(std::ostream& os, const node& elem)
    {
        os << elem.dump();
        return os;
    }

    void node::push(const node& value)
    {
        if (is<array>())
        {
            std::get<array>(m_data).push_back(value);
        }
        else
        {
            throw exceptions::invalid_cast(array_type, to_string(type()), VILI_EXC_INFO);
        }
    }

    void node::insert(size_t index, const node& value)
    {
        if (is<array>())
        {
            auto& vector = std::get<array>(m_data);
            vector.insert(vector.cbegin() + index, value);
        }
        else
        {
            throw exceptions::invalid_cast(array_type, to_string(type()), VILI_EXC_INFO);
        }
    }

    void node::insert(const std::string& key, node value)
    {
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            map.emplace(key, std::move(value));
        }
        else
        {
            throw exceptions::invalid_cast(object_type, to_string(type()), VILI_EXC_INFO);
        }
    }

    void node::erase(size_t index)
    {
        if (is<array>())
        {
            auto& vector = std::get<array>(m_data);
            if (index < vector.size())
            {
                vector.erase(vector.cbegin() + index);
            }
            else
            {
                throw exceptions::array_index_overflow(index, vector.size(), VILI_EXC_INFO);
            }
        }
        else
        {
            throw exceptions::invalid_cast(array_type, to_string(type()), VILI_EXC_INFO);
        }
    }

    void node::erase(size_t begin, size_t end)
    {
        if (is<array>())
        {
            auto& vector = std::get<array>(m_data);
            if (begin < vector.size() && end < vector.size())
            {
                vector.erase(vector.cbegin() + begin, vector.cbegin() + end);
            }
            else
            {
                throw exceptions::array_index_overflow(
                    begin, vector.size(), VILI_EXC_INFO); // TODO: Add end bound check
            }
        }
        else
        {
            throw exceptions::invalid_cast(array_type, to_string(type()), VILI_EXC_INFO);
        }
    }

    void node::erase(const std::string& key)
    {
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            if (const auto& element = map.find(key); element != map.end())
            {
                map.erase(element);
            }
            else
            {
                throw exceptions::unknown_child_node(key, VILI_EXC_INFO);
            }
        }
        else
        {
            throw exceptions::invalid_cast(object_type, to_string(type()), VILI_EXC_INFO);
        }
    }

    node& node::front()
    {
        if (is<array>())
        {
            return std::get<array>(m_data).front();
        }
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            return map.begin().value();
        }
        throw exceptions::invalid_cast(
            object_type, to_string(type()), VILI_EXC_INFO); // TODO: Add array
    }

    node& node::back()
    {
        if (is<array>())
        {
            return std::get<array>(m_data).back();
        }
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            auto& ref = (map.end() - 1).value();
            return ref;
        }
        throw exceptions::invalid_cast(
            object_type, to_string(type()), VILI_EXC_INFO); // TODO: Add array
    }

    node_iterator node::begin()
    {
        if (is<array>())
        {
            auto& vector = std::get<array>(m_data);
            return std::get<array>(m_data).data();
        }
        if (is<object>())
        {
            const auto* ptr = std::get<object>(m_data).data();
            return const_cast<object::value_type*>(ptr);
        }
    }

    node_iterator node::end()
    {
        if (is<array>())
        {
            auto& vector = std::get<array>(m_data);
            return vector.data() + vector.size();
        }
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            const auto ptr = map.data() + map.size();
            return const_cast<object::value_type*>(ptr);
        }
    }

    object& node::items()
    {
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            return map;
        }
    }

    node& node::at(const std::string& key)
    {
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            if (auto element = map.find(key); element != map.end())
            {
                return element.value();
            }
            else
            {
                throw exceptions::unknown_child_node(key, VILI_EXC_INFO);
            }
        }
        throw exceptions::invalid_cast(object_type, to_string(type()), VILI_EXC_INFO);
    }

    node& node::at(size_t index)
    {
        if (is<array>())
        {
            auto& vector = std::get<array>(m_data);
            if (index < vector.size())
            {
                return vector.at(index);
            }
            throw exceptions::array_index_overflow(index, vector.size(), VILI_EXC_INFO);
        }
        throw exceptions::invalid_cast(array_type, to_string(type()), VILI_EXC_INFO);
    }

    const node& node::at(const std::string& key) const
    {
        if (is<object>())
        {
            auto& map = std::get<object>(m_data);
            if (const auto element = map.find(key); element != map.end())
            {
                return element.value();
            }
            else
            {
                throw exceptions::unknown_child_node(key, VILI_EXC_INFO);
            }
        }
        throw exceptions::invalid_cast(object_type, to_string(type()), VILI_EXC_INFO);
    }

    const node& node::at(size_t index) const
    {
        if (is<array>())
        {
            auto& vector = std::get<array>(m_data);
            if (index < vector.size())
            {
                return vector.at(index);
            }
            throw exceptions::array_index_overflow(index, vector.size(), VILI_EXC_INFO);
        }
        throw exceptions::invalid_cast(array_type, to_string(type()), VILI_EXC_INFO);
    }

    node_data& node::data()
    {
        return m_data;
    }
}
# No Deep Nesting

Never write deeply nested `if` blocks. Use early returns, `continue`, and `break` to flatten logic.

Always add a blank line after `continue`, `return`, or `break` — unless it's the last statement in the block.

Bad:
```cpp
if (base_dict)
{
    auto it = base_dict->find(type);
    if (it != base_dict->end())
    {
        const auto * entry = it->second.find(id);
        if (entry != nullptr)
        {
            if (entry->original == original)
            {
                // deep logic here
            }
        }
    }
}
```

Good:
```cpp
if (!base_dict)
    return;

auto it = base_dict->find(type);
if (it == base_dict->end())
    return;

const auto * entry = it->second.find(id);
if (entry == nullptr)
    return;

if (entry->original != original)
    return;

// flat logic here
```

Bad (no blank line after continue):
```cpp
if (!esm.get_key().exist)
    continue;
if (esm.get_key().text != "sDefaultCellname")
    continue;
```

Good:
```cpp
if (!esm.get_key().exist)
    continue;

if (esm.get_key().text != "sDefaultCellname")
    continue;
```

Exception — last statement in block needs no blank line:
```cpp
if (!esm.get_value().exist)
    continue;
}
```

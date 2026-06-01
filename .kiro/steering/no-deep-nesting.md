# No Deep Nesting

Never write deeply nested `if` blocks. Use early returns, `continue`, and `break` to flatten logic.

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

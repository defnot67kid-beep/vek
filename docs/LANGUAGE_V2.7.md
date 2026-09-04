# VEK Language additions in 2.7

## For-each

```vek
fn total(values) {
    let result = 0;
    for value in values {
        result = result + value;
    }
    return result;
}
```

Arrays iterate values. Maps iterate values in sorted-key order for deterministic
behavior. Strings iterate one-character strings.

## Throw and catch

```vek
fn validate(speed) {
    if speed < 0 {
        throw "speed must be non-negative";
    }
    return speed;
}

fn main() {
    try {
        return validate(-1);
    } catch (error) {
        println("validation:", error);
        return 0;
    }
}
```

Only values explicitly raised with `throw` are catchable. Security-budget,
parser, native-host and VM runtime errors escape to the trusted host.

## Block comments

```vek
/* Multi-line comment.
   Useful for API documentation and disabled examples. */
```

## Structured VEK error values

Use `error(code, message, data)` to create a structured script-level error value.
`is_error(value)` checks whether a value was created by this helper. These values
can be raised with `throw` and recovered with `try/catch`.

```vek
fn load_part(id) {
    if id == "" {
        throw error("PART_ID_EMPTY", "part id is required", { id: id });
    }
    return id;
}
```

These are recoverable application errors and are deliberately separate from
trusted-host runtime/security diagnostics, which scripts cannot catch.

## Standard library additions

- `error(code, message, data)`, `is_error(value)`
- `assert(condition, message)`
- `array_get`, `array_set`, `array_pop`
- `map_keys`, `contains`
- `upper`, `lower`, `substring`, `starts_with`, `ends_with`
- `sin`, `cos`, `tan`, `log`, `exp`, `lerp`
- `time_ms`

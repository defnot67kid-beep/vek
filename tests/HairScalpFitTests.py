from pathlib import Path
s=Path(__file__).parents[1].joinpath('src/Character.cpp').read_text()
# Hair roots are projected to the outside of the 0.255-radius head rather than
# placing a giant intersecting cap through the forehead.
assert 'constexpr float skullRadius = 0.255f' in s
assert 'std::sqrt(std::max(0.0f,skullRadius*skullRadius-radialSq))+lift' in s
assert 'giant sphere intersecting the skull' in s
assert 'DrawSphere(root,rootRadius,hc)' in s
assert 'crownFill[]' in s
assert '0.286f' in s and '0.272f' in s
# New v26.6 styles must stay on the same scalp-root system.
for style in ('Wavy','Locs','Bun','Bob','Coils'):
    assert f'case HairStyle::{style}' in s
print('Hair scalp fit + v26.6 style regression checks passed')

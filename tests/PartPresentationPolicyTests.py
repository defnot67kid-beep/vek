from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
parts='\n'.join(p.read_text(encoding='utf-8') for p in sorted((root/'scripts'/'parts').glob('*.vek')))
renderer=(root/'src'/'PartPresentationRenderer.cpp').read_text(encoding='utf-8')
gui=(root/'scripts'/'vehicle_editor_gui.vek').read_text(encoding='utf-8')
bridge=(root/'src'/'VekVehicleEditorGuiSystem.cpp').read_text(encoding='utf-8')
make=(root/'src'/'VehicleBuilder.cpp').read_text(encoding='utf-8')
vehicle=(root/'src'/'Vehicle.cpp').read_text(encoding='utf-8')

registrations=re.findall(r'part_register\(\{id:"([^"]+)".*?presentation:part_presentation\("([^"]+)","([^"]+)","([^"]+)"',parts)
assert len(registrations) >= 28, f'expected presentation metadata for every part, got {len(registrations)}'
assert len({x[0] for x in registrations}) == len(registrations), 'duplicate part IDs'
for pid,icon,view,world in registrations:
    assert icon.startswith('icon:'), (pid, icon)
    assert view.startswith('builtin:'), (pid, view)
    assert world.startswith('builtin:'), (pid, world)
    model=view.split(':',1)[1]
    assert f'"{model}"' in renderer, f'{pid} view model not handled by CustomVehicle renderer: {model}'

by_id={x[0]:x for x in registrations}
assert by_id['seat.driver'][2] != by_id['tank.petrol_80'][2]
assert by_id['seat.driver'][1] != by_id['tank.petrol_80'][1]
assert by_id['battery.120'][2] == 'builtin:battery.pack120'
assert by_id['engine.small_petrol'][2] == 'builtin:engine.petrol'
assert 'p.icon' in gui and 'p.visual' not in re.search(r'gui_image_button\([^\n]+',gui).group(0)
assert 'm["icon"]=p.presentation.icon' in bridge
assert 'p.viewModel=d.presentation.viewModel' in make
assert 'PartPresentationRenderer::DrawModel' in make
assert 'PartPresentationRenderer::DrawModel' in vehicle
print(f'Part presentation policy tests: PASS ({len(registrations)} unique part presentations)')

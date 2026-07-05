import re

path = 'KmlArcGenerator_Standalone.html'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add handleSectorDrag and calculateBearing
bearing_logic = '''
function calculateBearing(lat1, lon1, lat2, lon2) {
    const toRad = Math.PI / 180;
    const toDeg = 180 / Math.PI;
    const l1 = lat1 * toRad;
    const l2 = lat2 * toRad;
    const dl = (lon2 - lon1) * toRad;
    
    const y = Math.sin(dl) * Math.cos(l2);
    const x = Math.cos(l1) * Math.sin(l2) - Math.sin(l1) * Math.cos(l2) * Math.cos(dl);
    let brng = Math.atan2(y, x) * toDeg;
    return (brng + 360) % 360;
}

function handleSectorDrag(e, type, id) {
    const sector = state.sectors.find(s => s.id === id);
    if (!sector) return;

    const lat = e.latlng.lat;
    const lon = e.latlng.lng;
    
    sector.radius = map.distance([sector.centerLat, sector.centerLon], [lat, lon]);
    const newAngle = calculateBearing(sector.centerLat, sector.centerLon, lat, lon);
    
    let delta = 0;
    if (type === 'start') {
        delta = newAngle - sector.startAngle;
    } else {
        delta = newAngle - sector.endAngle;
    }
    
    if (delta > 180) delta -= 360;
    if (delta < -180) delta += 360;
    
    sector.startAngle = (sector.startAngle + delta + 360) % 360;
    sector.endAngle = (sector.endAngle + delta + 360) % 360;
    
    // Sweep Angle 동기화
    let diff = sector.endAngle - sector.startAngle;
    if (diff < 0) diff += 360;
    sector.sweepAngle = diff;
    
    drawAllSectors();
}

function updateSectorForm(id) {
    const sector = state.sectors.find(s => s.id === id);
    if (!sector) return;
    
    const latDms = ddToDms(sector.centerLat, true);
    const lonDms = ddToDms(sector.centerLon, false);
    
    const parent = document.querySelector(`.sector-header[data-id="${id}"]`)?.nextElementSibling;
    if (parent) {
        parent.querySelector('.sec-input-latd').value = latDms.d;
        parent.querySelector('.sec-input-latm').value = latDms.m;
        parent.querySelector('.sec-input-lats').value = latDms.s;
        parent.querySelector('.sec-input-latdir').value = latDms.dir;
        
        parent.querySelector('.sec-input-lond').value = lonDms.d;
        parent.querySelector('.sec-input-lonm').value = lonDms.m;
        parent.querySelector('.sec-input-lons').value = lonDms.s;
        parent.querySelector('.sec-input-londir').value = lonDms.dir;
        
        parent.querySelector('.sec-input-radius').value = Math.round(sector.radius);
        parent.querySelector('.sec-input-start').value = Math.round(sector.startAngle * 100) / 100;
        parent.querySelector('.sec-input-end').value = Math.round(sector.endAngle * 100) / 100;
        parent.querySelector('.sec-input-sweep').value = Math.round(sector.sweepAngle * 100) / 100;
    }
}
'''
if 'function calculateBearing' not in content:
    # Insert before generateArcPoints
    content = content.replace('// 부채꼴 둘레의 점들 계산', bearing_logic + '\n// 부채꼴 둘레의 점들 계산')

# 2. Update drawAllSectors to add handles and change center drag to use updateSectorForm
draw_all_regex = r'// 모든 섹터 렌더링\nfunction drawAllSectors\(\) \{[\s\S]*?\}\n\}'
new_draw_all = '''// 모든 섹터 렌더링
function drawAllSectors() {
    const currentIds = state.sectors.map(s => s.id);
    for (const id in sectorLayers) {
        if (!currentIds.includes(id)) {
            if (sectorLayers[id].polygon) map.removeLayer(sectorLayers[id].polygon);
            if (sectorLayers[id].centerLine) map.removeLayer(sectorLayers[id].centerLine);
            if (sectorLayers[id].centerMarker) map.removeLayer(sectorLayers[id].centerMarker);
            if (sectorLayers[id].startHandle) map.removeLayer(sectorLayers[id].startHandle);
            if (sectorLayers[id].endHandle) map.removeLayer(sectorLayers[id].endHandle);
            delete sectorLayers[id];
        }
    }

    state.sectors.forEach(sector => {
        let eAngle = sector.endAngle;
        if (eAngle < sector.startAngle) eAngle += 360;
        
        const points = generateArcPoints(sector.centerLat, sector.centerLon, sector.radius, sector.startAngle, eAngle);
        const latlngs = points.map(p => [p.lat, p.lon]);

        if (!sectorLayers[sector.id]) {
            sectorLayers[sector.id] = { polygon: null, centerLine: null, centerMarker: null, startHandle: null, endHandle: null };
        }
        const layers = sectorLayers[sector.id];

        // Polygon
        if (!layers.polygon) {
            layers.polygon = L.polygon(latlngs, {
                color: sector.hexColor, weight: 2, fillColor: sector.hexColor, fillOpacity: sector.opacity / 100
            }).addTo(map);
        } else {
            layers.polygon.setLatLngs(latlngs);
            layers.polygon.setStyle({ color: sector.hexColor, fillColor: sector.hexColor, fillOpacity: sector.opacity / 100 });
        }

        // Center Line
        const midAngle = (sector.startAngle + eAngle) / 2;
        const midPos = getDestination(sector.centerLat, sector.centerLon, sector.radius, midAngle);
        if (!layers.centerLine) {
            layers.centerLine = L.polyline([[sector.centerLat, sector.centerLon], midPos], {
                color: sector.hexColor, weight: 1, dashArray: '5, 5'
            }).addTo(map);
        } else {
            layers.centerLine.setLatLngs([[sector.centerLat, sector.centerLon], midPos]);
            layers.centerLine.setStyle({ color: sector.hexColor });
        }

        // Center Marker
        if (!layers.centerMarker) {
            layers.centerMarker = L.marker([sector.centerLat, sector.centerLon], {icon: centerIcon, draggable: true}).addTo(map);
            layers.centerMarker.bindTooltip(sector.name, { permanent: true, direction: 'right', className: 'tooltip-label', offset: [5, 0] });
            layers.centerMarker.on('drag', function(e) {
                sector.centerLat = e.latlng.lat;
                sector.centerLon = e.latlng.lng;
                drawAllSectors();
                updateSectorForm(sector.id);
            });
        } else {
            layers.centerMarker.setLatLng([sector.centerLat, sector.centerLon]);
            layers.centerMarker.setTooltipContent(sector.name);
        }
        
        // Handles
        const handleIcon = L.divIcon({
            className: 'custom-div-icon',
            html: `<div style='background-color:${sector.hexColor}; width:10px; height:10px; border-radius:50%; border:2px solid white; box-shadow: 0 0 4px rgba(0,0,0,0.5); cursor: grab;'></div>`,
            iconSize: [10, 10],
            iconAnchor: [5, 5]
        });

        // 활성화된 섹터에만 핸들 표시
        if (state.activeSectorId === sector.id) {
            const startPos = getDestination(sector.centerLat, sector.centerLon, sector.radius, sector.startAngle);
            if (!layers.startHandle) {
                layers.startHandle = L.marker(startPos, {icon: handleIcon, draggable: true}).addTo(map);
                layers.startHandle.on('drag', function(e) { 
                    handleSectorDrag(e, 'start', sector.id); 
                    updateSectorForm(sector.id);
                });
            } else {
                layers.startHandle.setLatLng(startPos);
                layers.startHandle.setIcon(handleIcon);
            }

            const endPos = getDestination(sector.centerLat, sector.centerLon, sector.radius, eAngle);
            if (!layers.endHandle) {
                layers.endHandle = L.marker(endPos, {icon: handleIcon, draggable: true}).addTo(map);
                layers.endHandle.on('drag', function(e) { 
                    handleSectorDrag(e, 'end', sector.id); 
                    updateSectorForm(sector.id);
                });
            } else {
                layers.endHandle.setLatLng(endPos);
                layers.endHandle.setIcon(handleIcon);
            }
        } else {
            // 활성화되지 않은 섹터는 핸들 숨김
            if (layers.startHandle) {
                map.removeLayer(layers.startHandle);
                layers.startHandle = null;
            }
            if (layers.endHandle) {
                map.removeLayer(layers.endHandle);
                layers.endHandle = null;
            }
        }
    });
}'''

content = re.sub(draw_all_regex, new_draw_all, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Restored handle dragging.")

import re

path = 'KmlArcGenerator_Standalone.html'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Update addCustomMarker
add_marker_regex = r'function addCustomMarker\(lat, lon\) \{[\s\S]*?renderMarkerList\(\);\n\}'
new_add_marker = '''function addCustomMarker(lat, lon) {
    const id = Date.now().toString();
    const name = `마커 ${state.customMarkers.length + 1}`;
    state.customMarkers.push({ id, name, lat, lon });
    if (!state.showMarkers) {
        state.showMarkers = true;
        const toggle = document.getElementById('markerVisibilityToggle');
        if (toggle) toggle.checked = true;
    }
    syncMarkerMap();
    renderMarkerList();
}'''
content = re.sub(add_marker_regex, new_add_marker, content)

# 2. Update syncMarkerMap
sync_marker_regex = r'function syncMarkerMap\(\) \{[\s\S]*?\}\n\}'
new_sync_marker = '''function syncMarkerMap() {
    if (!markerLayerGroup) {
        markerLayerGroup = L.layerGroup().addTo(map);
    } else {
        markerLayerGroup.clearLayers();
    }
    if (!state.showMarkers) return;

    state.customMarkers.forEach((m, index) => {
        const iconHtml = `<div style="background-color:var(--accent-color); color:white; min-width:24px; height:24px; padding:0 4px; border-radius:12px; display:flex; align-items:center; justify-content:center; font-weight:bold; font-size:12px; border:2px solid white; box-shadow:0 2px 5px rgba(0,0,0,0.5); white-space:nowrap;">${index + 1}</div>`;
        const divIcon = L.divIcon({ html: iconHtml, className: '', iconSize: null, iconAnchor: [12, 12] });
        const marker = L.marker([m.lat, m.lon], { icon: divIcon, draggable: true }).addTo(markerLayerGroup);
        marker.bindTooltip(m.name || `마커 ${index + 1}`, { permanent: false, direction: 'top', offset: [0, -10] });
        
        // 지도 상 마커 드래그 이동
        marker.on('dragend', function(e) {
            m.lat = e.target.getLatLng().lat;
            m.lon = e.target.getLatLng().lng;
            renderMarkerList();
        });
        
        marker.on('click', (e) => { L.DomEvent.stopPropagation(e); });
    });
}'''
content = re.sub(sync_marker_regex, new_sync_marker, content)

# 3. Update renderMarkerList
render_list_regex = r'function renderMarkerList\(\) \{[\s\S]*?\}\);(\n\})?'
new_render_list = '''function renderMarkerList() {
    const listEl = document.getElementById('markerList');
    if (!listEl) return;
    
    // 포커스 복원용
    const activeElem = document.activeElement;
    let focusId = null;
    let focusClass = null;
    if (activeElem && activeElem.classList.contains('marker-name-edit')) {
        focusId = activeElem.dataset.id;
        focusClass = 'marker-name-edit';
    } else if (activeElem && activeElem.classList.contains('marker-dms-edit')) {
        focusId = activeElem.dataset.id;
        focusClass = Array.from(activeElem.classList).find(c => c.startsWith('m-'));
    }

    listEl.innerHTML = '';
    state.customMarkers.forEach((m, index) => {
        const item = document.createElement('div');
        item.className = 'marker-item';
        item.style.flexDirection = 'column';
        item.style.alignItems = 'flex-start';
        item.draggable = true;
        item.dataset.index = index;
        item.dataset.id = m.id;

        const latDms = ddToDms(m.lat, true);
        const lonDms = ddToDms(m.lon, false);

        item.innerHTML = `
            <div style="display:flex; width:100%; justify-content:space-between; align-items:center; margin-bottom: 0.3rem;">
                <div style="display:flex; align-items:center; gap: 0.5rem; flex:1;">
                    <span class="marker-num" style="cursor: grab;" title="드래그해서 순서 변경">≡ ${index + 1}</span>
                    <input type="text" class="marker-name-edit" data-id="${m.id}" value="${m.name || ''}" placeholder="마커 이름" style="flex:1; padding: 0.2rem 0.4rem; font-size:0.8rem; background: var(--bg-dark); color: white; border: 1px solid var(--border-color); border-radius: 4px;">
                </div>
                <button type="button" class="remove-btn" data-id="${m.id}" title="마커 삭제" style="margin-left: 0.5rem;">&times;</button>
            </div>
            <div class="dms-inputs" style="width:100%; margin-top:0.3rem;">
                <span style="font-size:0.75rem; width:20px;">위도</span>
                <input type="number" class="marker-dms-edit m-lat-d" value="${latDms.d}" min="0" max="90" data-id="${m.id}"><span>°</span>
                <input type="number" class="marker-dms-edit m-lat-m" value="${latDms.m}" min="0" max="59" data-id="${m.id}"><span>'</span>
                <input type="number" class="marker-dms-edit m-lat-s" value="${latDms.s}" min="0" max="59.99" step="any" data-id="${m.id}"><span>"</span>
                <select class="marker-dms-edit m-lat-dir" data-id="${m.id}" style="font-size:0.75rem; padding:0.2rem;">
                    <option value="N" ${latDms.dir === 'N' ? 'selected' : ''}>N</option>
                    <option value="S" ${latDms.dir === 'S' ? 'selected' : ''}>S</option>
                </select>
            </div>
            <div class="dms-inputs" style="width:100%; margin-top:0.3rem;">
                <span style="font-size:0.75rem; width:20px;">경도</span>
                <input type="number" class="marker-dms-edit m-lon-d" value="${lonDms.d}" min="0" max="180" data-id="${m.id}"><span>°</span>
                <input type="number" class="marker-dms-edit m-lon-m" value="${lonDms.m}" min="0" max="59" data-id="${m.id}"><span>'</span>
                <input type="number" class="marker-dms-edit m-lon-s" value="${lonDms.s}" min="0" max="59.99" step="any" data-id="${m.id}"><span>"</span>
                <select class="marker-dms-edit m-lon-dir" data-id="${m.id}" style="font-size:0.75rem; padding:0.2rem;">
                    <option value="E" ${lonDms.dir === 'E' ? 'selected' : ''}>E</option>
                    <option value="W" ${lonDms.dir === 'W' ? 'selected' : ''}>W</option>
                </select>
            </div>
        `;
        listEl.appendChild(item);
    });

    // 드래그 앤 드롭 이벤트
    listEl.querySelectorAll('.marker-item').forEach(item => {
        item.addEventListener('dragstart', (e) => {
            e.dataTransfer.setData('text/plain', item.dataset.index);
            item.style.opacity = '0.5';
        });
        item.addEventListener('dragend', (e) => {
            item.style.opacity = '1';
        });
        item.addEventListener('dragover', (e) => {
            e.preventDefault();
            item.style.borderTop = '2px solid var(--accent-color)';
        });
        item.addEventListener('dragleave', (e) => {
            item.style.borderTop = '';
        });
        item.addEventListener('drop', (e) => {
            e.preventDefault();
            item.style.borderTop = '';
            const fromIndex = parseInt(e.dataTransfer.getData('text/plain'));
            const toIndex = parseInt(item.dataset.index);
            if (fromIndex !== toIndex && !isNaN(fromIndex) && !isNaN(toIndex)) {
                const movedItem = state.customMarkers.splice(fromIndex, 1)[0];
                state.customMarkers.splice(toIndex, 0, movedItem);
                syncMarkerMap();
                renderMarkerList();
            }
        });
    });

    listEl.querySelectorAll('.remove-btn').forEach(btn => {
        btn.addEventListener('click', (e) => removeCustomMarker(e.target.dataset.id));
    });

    // 마커 이름 수정 이벤트
    listEl.querySelectorAll('.marker-name-edit').forEach(input => {
        input.addEventListener('change', (e) => {
            const id = e.target.dataset.id;
            const marker = state.customMarkers.find(m => m.id === id);
            if (marker) {
                marker.name = e.target.value;
                syncMarkerMap();
            }
        });
    });

    listEl.querySelectorAll('.marker-dms-edit').forEach(input => {
        input.addEventListener('change', (e) => {
            const id = e.target.dataset.id;
            const parent = e.target.closest('.marker-item');
            
            const latD = parseFloat(parent.querySelector('.m-lat-d').value) || 0;
            const latM = parseFloat(parent.querySelector('.m-lat-m').value) || 0;
            const latS = parseFloat(parent.querySelector('.m-lat-s').value) || 0;
            const latDir = parent.querySelector('.m-lat-dir').value;
            let lat = latD + (latM / 60) + (latS / 3600);
            if (latDir === 'S') lat = -lat;

            const lonD = parseFloat(parent.querySelector('.m-lon-d').value) || 0;
            const lonM = parseFloat(parent.querySelector('.m-lon-m').value) || 0;
            const lonS = parseFloat(parent.querySelector('.m-lon-s').value) || 0;
            const lonDir = parent.querySelector('.m-lon-dir').value;
            let lon = lonD + (lonM / 60) + (lonS / 3600);
            if (lonDir === 'W') lon = -lon;

            if (!isNaN(lat) && !isNaN(lon)) {
                updateCustomMarker(id, lat, lon);
            }
        });
    });

    // 포커스 복원
    if (focusId && focusClass) {
        const elem = document.querySelector(`.marker-item input.${focusClass}[data-id="${focusId}"]`);
        if (elem) elem.focus();
    }
}
'''
content = re.sub(render_list_regex, new_render_list, content)

# 4. Update generateKML
generate_kml_regex = r'(// 커스텀 마커 KML\n\s*customMarkers\.forEach\(\(m, idx\) => \{\n\s*placemarksKml \+= `\n\s*<Placemark>\n\s*<name>)(.*?)(</name>\n\s*<Point><coordinates>\$\{m\.lon\},\$\{m\.lat\},0</coordinates></Point>\n\s*</Placemark>`;\n\s*\}\);)'
def generate_replacement(m):
    return m.group(1) + '${m.name || "마커 " + (idx + 1)}' + m.group(3)

content = re.sub(generate_kml_regex, generate_replacement, content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Updated custom marker logic.")

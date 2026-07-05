
// 전역 변수
let map;
let markerLayerGroup = null;

// 앱 상태 (State) 관리
let state = {
    sectors: [], // { id, name, centerLat, centerLon, radius, startAngle, endAngle, sweepAngle, hexColor, opacity }
    kmlName: 'sectors',
    customMarkers: [], // 커스텀 마커 배열 [{id, lat, lon}]
    isMarkerMode: false, // 지도 클릭으로 마커 찍기 모드
    showMarkers: true, // 마커 지도 표시 여부
    activeSectorId: null // 아코디언이 열린 섹터
};

let sectorLayers = {}; // id -> { polygon, centerLine, centerMarker }

// 랜덤 밝은 색상 생성기
function getRandomColor() {
    const letters = '89ABCDEF';
    let color = '#';
    for (let i = 0; i < 6; i++) {
        color += letters[Math.floor(Math.random() * 8)];
    }
    return color;
}

// 고유 ID 생성기
function generateId() {
    return Date.now().toString() + Math.floor(Math.random() * 1000).toString();
}

// 기본 섹터 추가
function createDefaultSector() {
    return {
        id: generateId(),
        name: `Sector ${state.sectors.length + 1}`,
        centerLat: 37.5665,
        centerLon: 126.9780,
        radius: 500000,
        startAngle: 45,
        endAngle: 135,
        sweepAngle: 90,
        hexColor: getRandomColor(),
        opacity: 50
    };
}

document.addEventListener('DOMContentLoaded', () => {
    // 1. 지도 초기화
    map = L.map('map', { zoomControl: false }).setView([37.5665, 126.9780], 5);
    L.control.zoom({ position: 'topright' }).addTo(map);

    // 1.5. 패널 토글 로직
    const openBtn = document.getElementById('openPanelBtn');
    const panelCloseBtn = document.getElementById('closePanelBtn');
    const controlPanel = document.getElementById('controlPanel');

    openBtn.addEventListener('click', () => {
        controlPanel.classList.remove('hidden');
        openBtn.classList.remove('show');
        document.body.classList.remove('panel-hidden');
    });

    panelCloseBtn.addEventListener('click', () => {
        controlPanel.classList.add('hidden');
        openBtn.classList.add('show');
        document.body.classList.add('panel-hidden');
    });

    // 1.6. 커스텀 마커 로직 연동
    const markerModeToggle = document.getElementById('markerModeToggle');
    const markerVisibilityToggle = document.getElementById('markerVisibilityToggle');
    const addMarkerBtn = document.getElementById('addMarkerBtn');

    if (markerModeToggle) {
        markerModeToggle.addEventListener('change', (e) => {
            state.isMarkerMode = e.target.checked;
            document.getElementById('map').style.cursor = state.isMarkerMode ? 'crosshair' : '';
        });
    }

    if (markerVisibilityToggle) {
        markerVisibilityToggle.addEventListener('change', (e) => {
            state.showMarkers = e.target.checked;
            syncMarkerMap();
        });
    }

    map.on('click', (e) => {
        if (state.isMarkerMode) {
            addCustomMarker(e.latlng.lat, e.latlng.lng);
        } else {
            if (state.activeSectorId) {
                if (confirm("클릭한 위치로 선택된 부채꼴의 중심점을 이동하시겠습니까?")) {
                    const sector = state.sectors.find(s => s.id === state.activeSectorId);
                    if (sector) {
                        sector.centerLat = e.latlng.lat;
                        sector.centerLon = e.latlng.lng;
                        renderSectorList(); // 폼 갱신
                        drawAllSectors(); // 지도 갱신
                    }
                }
            }
        }
    });

    if (addMarkerBtn) {
        addMarkerBtn.addEventListener('click', () => {
            const latD = parseFloat(document.getElementById('mLatD').value) || 0;
            const latM = parseFloat(document.getElementById('mLatM').value) || 0;
            const latS = parseFloat(document.getElementById('mLatS').value) || 0;
            const latDir = document.getElementById('mLatDir').value;
            let lat = latD + (latM / 60) + (latS / 3600);
            if (latDir === 'S') lat = -lat;

            const lonD = parseFloat(document.getElementById('mLonD').value) || 0;
            const lonM = parseFloat(document.getElementById('mLonM').value) || 0;
            const lonS = parseFloat(document.getElementById('mLonS').value) || 0;
            const lonDir = document.getElementById('mLonDir').value;
            let lon = lonD + (lonM / 60) + (lonS / 3600);
            if (lonDir === 'W') lon = -lon;

            if (!isNaN(lat) && !isNaN(lon)) {
                addCustomMarker(lat, lon);
            } else {
                alert('유효한 위도와 경도를 입력해주세요.');
            }
        });
    }

    // 2. 기본 배경: 오프라인용 세계 국가 지도 (우선 즉시 렌더링)
    let offlineBaseLayer = null;
    if (typeof countriesData !== 'undefined') {
        offlineBaseLayer = L.geoJSON(countriesData, {
            style: {
                color: "#475569", 
                weight: 1.5,
                fillColor: "#0a0a0a", // 다크 모드
                fillOpacity: 1
            }
        }).addTo(map);
    }

    // 2-1. 하이브리드 체크
    const addSatelliteLayer = (isOfflineMode) => {
        if (offlineBaseLayer) map.removeLayer(offlineBaseLayer);
        
        L.TileLayer.Offline = L.TileLayer.extend({
            getTileUrl: function(coords) {
                const key = `${coords.z}_${coords.x}_${coords.y}`;
                if (typeof offlineTiles !== 'undefined' && offlineTiles[key]) {
                    return offlineTiles[key];
                } else if (!isOfflineMode) {
                    return L.TileLayer.prototype.getTileUrl.call(this, coords);
                } else {
                    return 'data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7'; // 투명 이미지
                }
            }
        });

        const satelliteLayer = new L.TileLayer.Offline('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
            maxZoom: 18,
            attribution: 'Tiles &copy; Esri'
        }).addTo(map);
        satelliteLayer.bringToBack();

        if (typeof countriesData !== 'undefined') {
            L.geoJSON(countriesData, {
                style: { color: "#ffffff", weight: 1.5, opacity: 0.4, fillOpacity: 0 }
            }).addTo(map);
        }
    };

    const testImg = new Image();
    testImg.onload = () => {
        document.title = "KML 부채꼴 생성기 (온라인 위성 모드)";
        const badge = document.getElementById('statusBadge');
        if (badge) {
            badge.textContent = "온라인 위성 모드";
            badge.className = "badge online";
        }
        addSatelliteLayer(false);
    };
    testImg.onerror = () => {
        document.title = "KML 부채꼴 생성기 (오프라인 모드)";
        const badge = document.getElementById('statusBadge');
        if (badge) {
            badge.textContent = "오프라인 모드";
            badge.className = "badge offline";
        }
        if (typeof offlineTiles !== 'undefined') {
            addSatelliteLayer(true);
            if (badge) {
                badge.textContent = "오프라인 위성 모드 (중동)";
                badge.style.background = "#0284c7";
            }
        }
    };
    testImg.src = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/0/0/0?_=" + new Date().getTime();

    if (typeof statesData !== 'undefined') {
        L.geoJSON(statesData, {
            style: { color: "#cbd5e1", weight: 0.8, dashArray: "3, 3", opacity: 0.5, fillOpacity: 0 }
        }).addTo(map);
    }

    if (typeof citiesData !== 'undefined') {
        L.geoJSON(citiesData, {
            pointToLayer: function (feature, latlng) {
                const isCapital = feature.properties.ADM0CAP === 1;
                const dotColor = isCapital ? "#cbd5e1" : "#4f4f4f";
                const radius = isCapital ? 3 : 2;
                return L.circleMarker(latlng, { radius: radius, fillColor: dotColor, color: "#0a0a0a", weight: 1, opacity: 1, fillOpacity: 1 });
            },
            onEachFeature: function (feature, layer) {
                if (feature.properties && feature.properties.NAME) {
                    const isCapital = feature.properties.ADM0CAP === 1;
                    const className = isCapital ? 'city-label capital-label' : 'city-label';
                    layer.bindTooltip(feature.properties.NAME, { permanent: isCapital, direction: 'right', className: className, offset: [5, 0] });
                }
            }
        }).addTo(map);
    }

    // 섹터 추가 이벤트
    document.getElementById('addSectorBtn').addEventListener('click', () => {
        const newSector = createDefaultSector();
        state.sectors.push(newSector);
        state.activeSectorId = newSector.id;
        renderSectorList();
        drawAllSectors();
    });

    // KML 이름 동기화
    document.getElementById('kmlName').addEventListener('input', (e) => {
        state.kmlName = e.target.value;
    });

    // KML 텍스트 생성 헬퍼
    function getKmlString() {
        return generateKML(state.kmlName, state.sectors, state.customMarkers);
    }

    // 다운로드
    document.getElementById('kmlForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const kmlContent = getKmlString();
        downloadKML(state.kmlName, kmlContent);
    });

    // KML 소스 모달
    const modal = document.getElementById('kmlModal');
    const viewBtn = document.getElementById('viewBtn');
    const closeBtn = document.querySelector('.close-btn');
    const copyBtn = document.getElementById('copyBtn');
    const kmlOutput = document.getElementById('kmlOutput');

    viewBtn.addEventListener('click', () => {
        kmlOutput.value = getKmlString();
        modal.classList.add('show');
    });

    closeBtn.addEventListener('click', () => {
        modal.classList.remove('show');
    });

    window.addEventListener('click', (e) => {
        if (e.target === modal) {
            modal.classList.remove('show');
        }
    });

    copyBtn.addEventListener('click', () => {
        kmlOutput.select();
        document.execCommand('copy');
        const originalText = copyBtn.textContent;
        copyBtn.textContent = '✅ 복사 완료!';
        copyBtn.style.background = '#10b981';
        setTimeout(() => {
            copyBtn.textContent = originalText;
            copyBtn.style.background = 'var(--accent-color)';
        }, 2000);
    });

    // 초기 실행
    if (state.sectors.length === 0) {
        const s = createDefaultSector();
        state.sectors.push(s);
        state.activeSectorId = s.id;
    }
    renderSectorList();
    drawAllSectors();
});

// 10진수 도(Degree) -> 도분초(DMS) 변환
function ddToDms(deg, isLat) {
    const dir = deg < 0 ? (isLat ? 'S' : 'W') : (isLat ? 'N' : 'E');
    let d = Math.abs(deg);
    let degOut = Math.floor(d);
    let m = (d - degOut) * 60;
    let minOut = Math.floor(m);
    let secOut = (m - minOut) * 60;
    return { d: degOut, m: minOut, s: secOut.toFixed(2), dir: dir };
}

// 오프라인 대응 중심점 아이콘
const centerIcon = L.divIcon({
    className: 'custom-div-icon',
    html: "<div style='background-color:#ef4444; width:12px; height:12px; border-radius:50%; border:2px solid white; box-shadow: 0 0 4px rgba(0,0,0,0.5); cursor: grab;'></div>",
    iconSize: [12, 12],
    iconAnchor: [6, 6]
});

// 모든 섹터 렌더링
function drawAllSectors() {
    // 삭제된 섹터의 레이어 정리
    const currentIds = state.sectors.map(s => s.id);
    for (const id in sectorLayers) {
        if (!currentIds.includes(id)) {
            if (sectorLayers[id].polygon) map.removeLayer(sectorLayers[id].polygon);
            if (sectorLayers[id].centerLine) map.removeLayer(sectorLayers[id].centerLine);
            if (sectorLayers[id].centerMarker) map.removeLayer(sectorLayers[id].centerMarker);
            delete sectorLayers[id];
        }
    }

    state.sectors.forEach(sector => {
        let eAngle = sector.endAngle;
        if (eAngle < sector.startAngle) eAngle += 360;
        
        const points = generateArcPoints(sector.centerLat, sector.centerLon, sector.radius, sector.startAngle, eAngle);
        const latlngs = points.map(p => [p.lat, p.lon]);

        if (!sectorLayers[sector.id]) {
            sectorLayers[sector.id] = { polygon: null, centerLine: null, centerMarker: null };
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

        // Center Marker with Tooltip for Name
        if (!layers.centerMarker) {
            layers.centerMarker = L.marker([sector.centerLat, sector.centerLon], {icon: centerIcon, draggable: true}).addTo(map);
            layers.centerMarker.bindTooltip(sector.name, { permanent: true, direction: 'right', className: 'tooltip-label', offset: [5, 0] });
            layers.centerMarker.on('drag', function(e) {
                sector.centerLat = e.latlng.lat;
                sector.centerLon = e.latlng.lng;
                renderSectorList();
                drawAllSectors();
            });
        } else {
            layers.centerMarker.setLatLng([sector.centerLat, sector.centerLon]);
            layers.centerMarker.setTooltipContent(sector.name);
        }
    });
}

// 부채꼴 둘레의 점들 계산
function generateArcPoints(lat, lon, radius, startAngle, endAngle) {
    const R = 6378137; 
    const lat1 = lat * Math.PI / 180;
    const lon1 = lon * Math.PI / 180;
    const d = radius / R; 

    const points = [];
    points.push({lat: lat, lon: lon});

    const step = 1; 
    for (let angle = startAngle; angle <= endAngle; angle += step) {
        const brng = angle * Math.PI / 180;
        const lat2 = Math.asin(Math.sin(lat1) * Math.cos(d) + Math.cos(lat1) * Math.sin(d) * Math.cos(brng));
        const lon2 = lon1 + Math.atan2(Math.sin(brng) * Math.sin(d) * Math.cos(lat1), Math.cos(d) - Math.sin(lat1) * Math.sin(lat2));
        points.push({ lat: lat2 * 180 / Math.PI, lon: lon2 * 180 / Math.PI });
    }

    if ((endAngle - startAngle) % step !== 0) {
        const brng = endAngle * Math.PI / 180;
        const lat2 = Math.asin(Math.sin(lat1) * Math.cos(d) + Math.cos(lat1) * Math.sin(d) * Math.cos(brng));
        const lon2 = lon1 + Math.atan2(Math.sin(brng) * Math.sin(d) * Math.cos(lat1), Math.cos(d) - Math.sin(lat1) * Math.sin(lat2));
        points.push({ lat: lat2 * 180 / Math.PI, lon: lon2 * 180 / Math.PI });
    }

    points.push({lat: lat, lon: lon});
    return points;
}

// 거리 계산 유틸
function getDestination(lat, lon, distance, bearing) {
    const R = 6378137; 
    const lat1 = lat * Math.PI / 180;
    const lon1 = lon * Math.PI / 180;
    const d = distance / R; 
    const brng = bearing * Math.PI / 180;
    const lat2 = Math.asin(Math.sin(lat1) * Math.cos(d) + Math.cos(lat1) * Math.sin(d) * Math.cos(brng));
    const lon2 = lon1 + Math.atan2(Math.sin(brng) * Math.sin(d) * Math.cos(lat1), Math.cos(d) - Math.sin(lat1) * Math.sin(lat2));
    return [lat2 * 180 / Math.PI, lon2 * 180 / Math.PI];
}

// UI 렌더링
function renderSectorList() {
    const listEl = document.getElementById('sectorList');
    if (!listEl) return;
    
    // 포커스 유지
    const activeElem = document.activeElement;
    let focusClass = null;
    if (activeElem && activeElem.classList.length > 0 && activeElem.closest('.sector-item')) {
        focusClass = Array.from(activeElem.classList).find(c => c.startsWith('sec-input-'));
    }

    listEl.innerHTML = '';
    
    state.sectors.forEach((s, idx) => {
        const isOpen = state.activeSectorId === s.id;
        const latDms = ddToDms(s.centerLat, true);
        const lonDms = ddToDms(s.centerLon, false);

        const item = document.createElement('div');
        item.className = 'sector-item';
        
        item.innerHTML = `
            <div class="sector-header" data-id="${s.id}">
                <h3><span class="sector-color-badge" style="background-color: ${s.hexColor};"></span> ${s.name}</h3>
                <div style="display:flex; align-items:center; gap:10px;">
                    <span style="font-size:0.8rem; color:var(--text-secondary);">${isOpen ? '▲' : '▼'}</span>
                    ${state.sectors.length > 1 ? `<button type="button" class="remove-sector-btn" data-id="${s.id}" title="원호 삭제">&times;</button>` : ''}
                </div>
            </div>
            <div class="sector-body ${isOpen ? 'open' : ''}">
                <div class="settings-group">
                    <div class="input-box" style="flex:2;">
                        <label>이름</label>
                        <input type="text" class="sec-input-name" data-id="${s.id}" value="${s.name}">
                    </div>
                    <div class="input-box" style="flex:1;">
                        <label>색상</label>
                        <input type="color" class="sec-input-color" data-id="${s.id}" value="${s.hexColor}">
                    </div>
                    <div class="input-box" style="flex:1;">
                        <label>투명도(%)</label>
                        <input type="number" class="sec-input-opacity" data-id="${s.id}" value="${s.opacity}" min="0" max="100">
                    </div>
                </div>

                <label style="font-size:0.85rem; color:var(--text-secondary); display:block; margin-bottom:0.4rem; font-weight:500;">위도</label>
                <div class="dms-inputs" style="margin-bottom:0.8rem;">
                    <input type="number" class="sec-input-latd" data-id="${s.id}" value="${latDms.d}"><span>°</span>
                    <input type="number" class="sec-input-latm" data-id="${s.id}" value="${latDms.m}"><span>'</span>
                    <input type="number" step="any" class="sec-input-lats" data-id="${s.id}" value="${latDms.s}"><span>"</span>
                    <select class="sec-input-latdir" data-id="${s.id}">
                        <option value="N" ${latDms.dir === 'N' ? 'selected' : ''}>N</option>
                        <option value="S" ${latDms.dir === 'S' ? 'selected' : ''}>S</option>
                    </select>
                </div>

                <label style="font-size:0.85rem; color:var(--text-secondary); display:block; margin-bottom:0.4rem; font-weight:500;">경도</label>
                <div class="dms-inputs" style="margin-bottom:0.8rem;">
                    <input type="number" class="sec-input-lond" data-id="${s.id}" value="${lonDms.d}"><span>°</span>
                    <input type="number" class="sec-input-lonm" data-id="${s.id}" value="${lonDms.m}"><span>'</span>
                    <input type="number" step="any" class="sec-input-lons" data-id="${s.id}" value="${lonDms.s}"><span>"</span>
                    <select class="sec-input-londir" data-id="${s.id}">
                        <option value="E" ${lonDms.dir === 'E' ? 'selected' : ''}>E</option>
                        <option value="W" ${lonDms.dir === 'W' ? 'selected' : ''}>W</option>
                    </select>
                </div>

                <div class="settings-group">
                    <div class="input-box">
                        <label>반지름(m)</label>
                        <input type="number" class="sec-input-radius" data-id="${s.id}" value="${Math.round(s.radius)}">
                    </div>
                    <div class="input-box">
                        <label>원호(범위)°</label>
                        <input type="number" class="sec-input-sweep" data-id="${s.id}" value="${Math.round(s.sweepAngle * 100)/100}">
                    </div>
                </div>
                <div class="settings-group" style="margin-bottom:0;">
                    <div class="input-box">
                        <label>시작 방위각°</label>
                        <input type="number" class="sec-input-start" data-id="${s.id}" value="${Math.round(s.startAngle * 100)/100}">
                    </div>
                    <div class="input-box">
                        <label>종료 방위각°</label>
                        <input type="number" class="sec-input-end" data-id="${s.id}" value="${Math.round(s.endAngle * 100)/100}">
                    </div>
                </div>
            </div>
        `;
        listEl.appendChild(item);
    });

    // 헤더 클릭 (아코디언 토글)
    listEl.querySelectorAll('.sector-header').forEach(header => {
        header.addEventListener('click', (e) => {
            if (e.target.classList.contains('remove-sector-btn')) return;
            const id = header.dataset.id;
            state.activeSectorId = state.activeSectorId === id ? null : id;
            renderSectorList();
        });
    });

    // 삭제 버튼
    listEl.querySelectorAll('.remove-sector-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const id = e.target.dataset.id;
            state.sectors = state.sectors.filter(s => s.id !== id);
            if (state.activeSectorId === id) state.activeSectorId = null;
            renderSectorList();
            drawAllSectors();
        });
    });

    // 인풋 이벤트 리스너
    listEl.querySelectorAll('input, select').forEach(input => {
        input.addEventListener('input', (e) => {
            const id = e.target.dataset.id;
            const sector = state.sectors.find(s => s.id === id);
            if (!sector) return;

            const parent = e.target.closest('.sector-body');
            sector.name = parent.querySelector('.sec-input-name').value;
            sector.hexColor = parent.querySelector('.sec-input-color').value;
            sector.opacity = parseFloat(parent.querySelector('.sec-input-opacity').value) || 50;
            
            const latD = parseFloat(parent.querySelector('.sec-input-latd').value) || 0;
            const latM = parseFloat(parent.querySelector('.sec-input-latm').value) || 0;
            const latS = parseFloat(parent.querySelector('.sec-input-lats').value) || 0;
            const latDir = parent.querySelector('.sec-input-latdir').value;
            sector.centerLat = latD + (latM / 60) + (latS / 3600);
            if (latDir === 'S') sector.centerLat = -sector.centerLat;

            const lonD = parseFloat(parent.querySelector('.sec-input-lond').value) || 0;
            const lonM = parseFloat(parent.querySelector('.sec-input-lonm').value) || 0;
            const lonS = parseFloat(parent.querySelector('.sec-input-lons').value) || 0;
            const lonDir = parent.querySelector('.sec-input-londir').value;
            sector.centerLon = lonD + (lonM / 60) + (lonS / 3600);
            if (lonDir === 'W') sector.centerLon = -sector.centerLon;

            sector.radius = parseFloat(parent.querySelector('.sec-input-radius').value) || 0;
            sector.startAngle = parseFloat(parent.querySelector('.sec-input-start').value) || 0;

            const endInput = parseFloat(parent.querySelector('.sec-input-end').value) || 0;
            const sweepInput = parseFloat(parent.querySelector('.sec-input-sweep').value) || 0;

            if (e.target.classList.contains('sec-input-sweep')) {
                sector.sweepAngle = sweepInput;
                sector.endAngle = (sector.startAngle + sector.sweepAngle) % 360;
                parent.querySelector('.sec-input-end').value = Math.round(sector.endAngle * 100) / 100;
            } else {
                sector.endAngle = endInput;
                let diff = sector.endAngle - sector.startAngle;
                if (diff < 0) diff += 360;
                sector.sweepAngle = diff;
                parent.querySelector('.sec-input-sweep').value = Math.round(sector.sweepAngle * 100) / 100;
            }
            
            // 즉시 렌더링하면 포커스가 날아가므로 지도만 업데이트
            // 폼은 현재 상태를 유지하되 데이터만 갱신
            drawAllSectors();
        });
    });

    // 포커스 복원 (만약 렌더링이 강제로 일어났을 때)
    if (focusClass && state.activeSectorId) {
        const elem = document.querySelector(`.sector-item .${focusClass}[data-id="${state.activeSectorId}"]`);
        if (elem) elem.focus();
    }
}

// ----------------------------------------------------
// KML 다중 생성 로직
// ----------------------------------------------------
function generateKML(docName, sectors, customMarkers) {
    let placemarksKml = '';

    // 부채꼴들 KML
    sectors.forEach(sector => {
        const r = sector.hexColor.substring(1, 3);
        const g = sector.hexColor.substring(3, 5);
        const b = sector.hexColor.substring(5, 7);
        const alphaHex = Math.round((sector.opacity / 100) * 255).toString(16).padStart(2, '0').toUpperCase();
        const kmlColor = `${alphaHex}${b}${g}${r}`.toLowerCase();
        const lineColor = `ff${b}${g}${r}`.toLowerCase();

        let eAngle = sector.endAngle;
        if (eAngle < sector.startAngle) eAngle += 360;
        const points = generateArcPoints(sector.centerLat, sector.centerLon, sector.radius, sector.startAngle, eAngle);
        const coordsStr = points.map(p => `${p.lon},${p.lat},0`).join(' ');

        placemarksKml += `
    <Style id="sectorStyle_${sector.id}">
      <LineStyle><color>${lineColor}</color><width>2</width></LineStyle>
      <PolyStyle><color>${kmlColor}</color><fill>1</fill><outline>1</outline></PolyStyle>
    </Style>
    <Placemark>
      <name>${sector.name}</name>
      <styleUrl>#sectorStyle_${sector.id}</styleUrl>
      <Polygon>
        <extrude>0</extrude><altitudeMode>clampToGround</altitudeMode>
        <outerBoundaryIs><LinearRing><coordinates>${coordsStr}</coordinates></LinearRing></outerBoundaryIs>
      </Polygon>
    </Placemark>
    <Placemark>
      <name>${sector.name} 중심점</name>
      <Point><coordinates>${sector.centerLon},${sector.centerLat},0</coordinates></Point>
    </Placemark>`;
    });

    // 커스텀 마커 KML
    customMarkers.forEach((m, idx) => {
        placemarksKml += `
    <Placemark>
      <name>${m.name || "마커 " + (idx + 1)}</name>
      <Point><coordinates>${m.lon},${m.lat},0</coordinates></Point>
    </Placemark>`;
    });

    return `<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <name>${docName}.kml</name>
${placemarksKml}
  </Document>
</kml>`;
}

// 다운로드 기능
function downloadKML(filename, content) {
    const blob = new Blob([content], { type: 'application/vnd.google-earth.kml+xml' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${filename}.kml`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

// ----------------------------------------------------
// 커스텀 마커 로직
// ----------------------------------------------------
function addCustomMarker(lat, lon) {
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
}

function removeCustomMarker(id) {
    state.customMarkers = state.customMarkers.filter(m => m.id !== id);
    syncMarkerMap();
    renderMarkerList();
}

function updateCustomMarker(id, lat, lon) {
    const marker = state.customMarkers.find(m => m.id === id);
    if (marker) {
        marker.lat = lat;
        marker.lon = lon;
        syncMarkerMap();
    }
}

function syncMarkerMap() {
    if (!markerLayerGroup) {
        markerLayerGroup = L.layerGroup().addTo(map);
    } else {
        markerLayerGroup.clearLayers();
    }
    if (!state.showMarkers) return;

    state.customMarkers.forEach((m, index) => {
        const iconHtml = `<div style="background-color:var(--accent-color); color:white; min-width:24px; height:24px; padding:0 4px; border-radius:12px; display:flex; align-items:center; justify-content:center; font-weight:bold; font-size:12px; border:2px solid white; box-shadow:0 2px 5px rgba(0,0,0,0.5); white-space:nowrap;">${index + 1}</div>`;
        const divIcon = L.divIcon({ html: iconHtml, className: '', iconSize: null, iconAnchor: [12, 12] });
        const marker = L.marker([m.lat, m.lon], { icon: divIcon }).addTo(markerLayerGroup);
        marker.on('click', (e) => { L.DomEvent.stopPropagation(e); });
    });
}

function renderMarkerList() {
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

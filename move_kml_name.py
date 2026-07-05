import re

path = 'KmlArcGenerator_Standalone.html'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. 추출할 '출력 파일명' 블록
kml_name_block_pattern = r'(\s*<div class="settings-group"[^>]*>\s*<div class="input-box">\s*<label for="kmlName">출력 파일명</label>\s*<input type="text" id="kmlName" value="sectors">\s*</div>\s*</div>)'
match = re.search(kml_name_block_pattern, content)

if match:
    kml_name_block = match.group(1)
    
    # 먼저 해당 블록을 원래 위치에서 제거
    content = content.replace(kml_name_block, '')
    
    # 2. 삽입할 위치 찾기: <div class="button-group"> 이전
    button_group_pattern = r'(\s*<div class="button-group">)'
    
    # 새로운 출력 파일명 UI 블록을 카드로 감싸기
    new_kml_name_card = '''
                    <div class="card" style="padding: 1rem;">
                        <div class="settings-group" style="margin: 0;">
                            <div class="input-box" style="flex: 1;">
                                <label for="kmlName">출력 파일명</label>
                                <input type="text" id="kmlName" value="sectors">
                            </div>
                        </div>
                    </div>'''
    
    content = re.sub(button_group_pattern, new_kml_name_card + r'\1', content)
    
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("Successfully moved KML Name box.")
else:
    print("Could not find KML Name box.")

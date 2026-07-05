import os

filepath = 'KmlArcGenerator_Standalone.html'
with open(filepath, 'r', encoding='utf-8') as f:
    lines = f.readlines()

new_lines = []
for i, line in enumerate(lines):
    line_num = i + 1
    
    # offlineTiles (1318 ~ 5924)
    if 1318 <= line_num <= 5924:
        if line_num == 1318:
            new_lines.append('const offlineTiles = undefined;\n')
        continue
        
    # countriesData (6096 ~ 6361)
    if 6096 <= line_num <= 6361:
        if line_num == 6096:
            new_lines.append('const countriesData = undefined;\n')
        continue
        
    # statesData (6364 ~ 6365)
    if 6364 <= line_num <= 6365:
        if line_num == 6364:
            new_lines.append('const statesData = undefined;\n')
        continue
        
    # citiesData (6368 ~ 6369)
    if 6368 <= line_num <= 6369:
        if line_num == 6368:
            new_lines.append('const citiesData = undefined;\n')
        continue
        
    new_lines.append(line)

with open(filepath, 'w', encoding='utf-8') as f:
    f.writelines(new_lines)

print("Data removed successfully.")

import re

orig_path = '../KmlArcGenerator_Standalone.html'
cur_path = 'KmlArcGenerator_Standalone.html'

with open(orig_path, 'r', encoding='utf-8') as f:
    orig_content = f.read()

with open(cur_path, 'r', encoding='utf-8') as f:
    cur_content = f.read()

def get_var_def(var_name):
    # Match from const var_name = until the matching semicolon or end of script tag
    match = re.search(f'(const {var_name}\s*=\s*.*?;)', orig_content, re.DOTALL)
    if match:
        return match.group(1)
    return None

countries_def = get_var_def('countriesData')
states_def = get_var_def('statesData')
cities_def = get_var_def('citiesData')

if countries_def:
    cur_content = re.sub(r'const countriesData\s*=\s*undefined;', countries_def, cur_content, flags=re.DOTALL)

if states_def:
    cur_content = re.sub(r'const statesData\s*=\s*undefined;', states_def, cur_content, flags=re.DOTALL)

if cities_def:
    cur_content = re.sub(r'const citiesData\s*=\s*undefined;', cities_def, cur_content, flags=re.DOTALL)

with open(cur_path, 'w', encoding='utf-8') as f:
    f.write(cur_content)

print("Data restored successfully.")

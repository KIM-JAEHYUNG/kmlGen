import re

path = 'KmlArcGenerator_Standalone.html'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# We need to remove the leftover code after the injected renderMarkerList()
# The leftover code starts with:
#     listEl.querySelectorAll('.remove-btn').forEach(btn => {
# and ends right before </script>

# Search for the string after the injected function:
leftover_pattern = r'(\}\n\s*listEl\.querySelectorAll\(\'\.remove-btn\'\)[\s\S]*?\n\})(\n</script>)'

content, count = re.subn(leftover_pattern, r'}\2', content)

if count > 0:
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"Fixed {count} instances of leftover code.")
else:
    print("Could not find the leftover code to remove.")

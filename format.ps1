echo "formatting..."
clang-format -style=file:.clang-format -i $(Get-ChildItem -Path $PWD/Source -Recurse | Where Name -Match '.(?:cpp|h)$' | Select-Object -ExpandProperty FullName)
echo "-> done!"
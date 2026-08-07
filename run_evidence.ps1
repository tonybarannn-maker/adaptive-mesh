$ErrorActionPreference = "Stop"

Write-Host "[1/3] Компіляція проєкту..."
cmake --build build --config Release

Write-Host "[2/3] Створення директорії результатів та запуск тестів..."
New-Item -ItemType Directory -Force results | Out-Null
.\build\mesh_tests.exe

Write-Host "[3/3] Генерація криптографічного SHA-256 маніфесту..."
Get-FileHash .\results\runtime_trace.jsonl -Algorithm SHA256 | Out-File .\verification_manifest.sha256 -Encoding utf8

Write-Host "Evidence generated successfully!" -ForegroundColor Green

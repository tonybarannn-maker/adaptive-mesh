$ErrorActionPreference = "Stop"

Write-Host "[1/3] Компіляція проєкту..."
cmake --build build --config Release

Write-Host "[2/3] Створення директорії результатів та запуск тестів..."
$resultsDir = Join-Path $PSScriptRoot "results"
New-Item -ItemType Directory -Force $resultsDir | Out-Null

# Запускаємо з повним шляхом до екзешника
$exePath = Join-Path $PSScriptRoot "build\mesh_tests.exe"
& $exePath

Write-Host "[3/3] Генерація криптографічного SHA-256 маніфесту..."
$tracePath = Join-Path $resultsDir "runtime_trace.jsonl"
$manifestPath = Join-Path $PSScriptRoot "verification_manifest.sha256"

if (Test-Path $tracePath) {
    Get-FileHash $tracePath -Algorithm SHA256 | Out-File $manifestPath -Encoding utf8
    Write-Host "Evidence generated successfully!" -ForegroundColor Green
} else {
    Write-Error "Помилка: файл runtime_trace.jsonl не знайдено за шляхом $tracePath"
}


$ErrorActionPreference = 'Stop'

Write-Host "Current path: $((Get-Location).Path)"
if ((Get-Location).Path -match '[^\x00-\x7F]') {
    Write-Host 'ASCII path: NO'
} else {
    Write-Host 'ASCII path: YES'
}

Write-Host ''
Write-Host 'app encoding:'
Get-ChildItem app\*.hpp,app\*.cpp | ForEach-Object {
    $b = [IO.File]::ReadAllBytes($_.FullName)
    $s = [Text.Encoding]::UTF8.GetString($b)
    $bad = $s.Contains([char]0xFFFD)
    $bom = ($b.Length -ge 3 -and $b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF)
    "{0,-26} {1,-8} BOM={2}" -f $_.Name, $(if($bad){'BAD'}else{'UTF8'}), $bom
}

Write-Host ''
Write-Host 'Qt includes in core/ and libs/ (expected empty):'
$qtIncludes = rg -n '#include\s*[<"]Q' core libs 2>$null
if ($LASTEXITCODE -eq 0) { $qtIncludes } else { Write-Host '(empty)' }

Write-Host ''
Write-Host 'git diff --stat libs/messaging/ (expected empty):'
git diff --stat -- libs/messaging/

Read-Host 'Press Enter to exit'

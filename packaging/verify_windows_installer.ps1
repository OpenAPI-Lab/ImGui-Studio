[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$Installer,
    [switch]$PreflightOnly
)

$ErrorActionPreference = 'Stop'

$productKeys = @(
    'Software\ImGui Studio\ImGui Studio'
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\ImGui Studio'
)

function Get-ProductRegistryEntries {
    foreach ($hive in 'LocalMachine', 'CurrentUser') {
        foreach ($view in 'Registry32', 'Registry64') {
            $baseKey = [Microsoft.Win32.RegistryKey]::OpenBaseKey($hive, $view)
            try {
                foreach ($productKey in $productKeys) {
                    $key = $baseKey.OpenSubKey($productKey)
                    if ($null -ne $key) {
                        try {
                            [pscustomobject]@{
                                Location = "$hive ($view)\$productKey"
                                Key = $productKey
                                InstallPath = $key.GetValue('')
                                UninstallCommand = $key.GetValue('UninstallString')
                            }
                        } finally {
                            $key.Dispose()
                        }
                    }
                }
            } finally {
                $baseKey.Dispose()
            }
        }
    }
}

function Get-ExistingPaths($Paths) {
    foreach ($candidate in $Paths) {
        if (Test-Path -LiteralPath $candidate) { $candidate }
    }
}

$package = Get-Item -LiteralPath $Installer
if ($package.PSIsContainer -or $package.Name -notlike 'ImGuiStudio-*-Windows-*.exe' -or
    $package.VersionInfo.ProductName -ne 'ImGui Studio' -or
    $package.VersionInfo.FileDescription -ne 'ImGui Studio Installer') {
    throw "El archivo no es un instalador generado de ImGui Studio: $Installer"
}
if ($package.Directory.Name -ne 'packages') {
    throw 'Usa el instalador dentro de la carpeta packages generada por CPack.'
}
$expectedHash = (Get-Content -LiteralPath ($package.FullName + '.sha256') -TotalCount 1) -split '\s+' |
    Select-Object -First 1
if ($expectedHash -notmatch '^[0-9a-fA-F]{64}$' -or
    (Get-FileHash -LiteralPath $package.FullName -Algorithm SHA256).Hash -ne $expectedHash) {
    throw 'El SHA-256 del instalador no coincide con el archivo generado por CPack.'
}

$commonPrograms = [Environment]::GetFolderPath('CommonPrograms')
if ([string]::IsNullOrEmpty($commonPrograms)) { throw 'Windows no devolvio la ruta del menu Inicio comun.' }
$menuGroup = Join-Path $commonPrograms 'ImGui Studio'
$shortcutPaths = @($menuGroup)
foreach ($folder in 'Programs', 'DesktopDirectory', 'CommonDesktopDirectory') {
    $folderPath = [Environment]::GetFolderPath($folder)
    if ($folderPath) {
        $name = if ($folder -eq 'Programs') { 'ImGui Studio' } else { 'ImGui Studio.lnk' }
        $shortcutPaths += Join-Path $folderPath $name
    }
}
$defaultInstallPaths = @($env:ProgramFiles, ${env:ProgramFiles(x86)}, $env:ProgramW6432) |
    Where-Object { $_ } | Sort-Object -Unique | ForEach-Object { Join-Path $_ 'ImGui Studio' }
$conflicts = @((Get-ProductRegistryEntries).Location) + @(Get-ExistingPaths ($shortcutPaths + $defaultInstallPaths))
$conflicts = @($conflicts | Where-Object { $_ })
if ($conflicts.Count) { throw "Se encontraron rastros previos de ImGui Studio:`n$($conflicts -join "`n")" }
Write-Output 'SHA-256 verificado y comprobacion previa sin conflictos.'
if ($PreflightOnly) { return }

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Ejecuta la prueba de instalacion desde una terminal de administrador.'
}

$workRoot = $package.Directory.Parent.FullName
$stage = Join-Path $workRoot ('verify-install-' + [Guid]::NewGuid().ToString('N'))
if (Test-Path -LiteralPath $stage) { throw "La ruta de prueba ya existe: $stage" }
$editor = Join-Path $stage 'bin\imgui_studio.exe'
$uninstaller = Join-Path $stage 'Uninstall.exe'

try {
    $install = Start-Process -FilePath $package.FullName -ArgumentList '/S', "/D=$stage" `
        -WorkingDirectory $workRoot -WindowStyle Hidden -Wait -PassThru
    if ($install.ExitCode -ne 0) { throw "El instalador devolvio $($install.ExitCode)." }

    $entries = @(Get-ProductRegistryEntries)
    if ($stage -notin $entries.InstallPath -or ('"' + $uninstaller + '"') -notin $entries.UninstallCommand) {
        throw 'Windows no registro la carpeta o el desinstalador de esta prueba.'
    }
    if (-not (Test-Path -LiteralPath $editor -PathType Leaf) -or
        -not (Test-Path -LiteralPath $uninstaller -PathType Leaf)) {
        throw 'No se encontraron el editor y el desinstalador en la carpeta de prueba.'
    }
    $projectLicense = Join-Path $PSScriptRoot '..\LICENSE'
    $installedLicense = Join-Path $stage 'share\doc\ImGuiStudioNative\LICENSE.txt'
    if ((Get-FileHash -LiteralPath $projectLicense).Hash -ne (Get-FileHash -LiteralPath $installedLicense).Hash) {
        throw 'La licencia instalada no coincide con la licencia del proyecto.'
    }

    $shell = New-Object -ComObject WScript.Shell
    $expectedShortcuts = @{ 'ImGui Studio.lnk' = $editor; 'Uninstall.lnk' = $uninstaller }
    foreach ($name in $expectedShortcuts.Keys) {
        $shortcut = Join-Path $menuGroup $name
        if (-not (Test-Path -LiteralPath $shortcut -PathType Leaf) -or
            $shell.CreateShortcut($shortcut).TargetPath -ne $expectedShortcuts[$name]) {
            throw "El acceso directo no apunta al archivo esperado: $shortcut"
        }
    }

    $originalPath = $env:Path
    try {
        $env:Path = "$env:SystemRoot\System32;$env:SystemRoot"
        $smoke = Start-Process -FilePath $editor -ArgumentList '--smoke-test' `
            -WorkingDirectory (Split-Path -Parent $editor) -WindowStyle Hidden -Wait -PassThru
        if ($smoke.ExitCode -ne 0) { throw "El editor instalado devolvio $($smoke.ExitCode)." }
    } finally {
        $env:Path = $originalPath
    }
    Write-Output 'Instalacion, registro, accesos directos y arranque verificados.'
} finally {
    if (Test-Path -LiteralPath $uninstaller -PathType Leaf) {
        $installedFile = Get-Item -LiteralPath $uninstaller
        if ($installedFile.Directory.FullName -ne $stage -or
            ($installedFile.Attributes -band [IO.FileAttributes]::ReparsePoint) -or
            ($installedFile.Directory.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "La ruta del desinstalador no corresponde a la carpeta temporal: $uninstaller"
        }
        $uninstall = Start-Process -FilePath $uninstaller -ArgumentList '/S' `
            -WorkingDirectory $workRoot -WindowStyle Hidden -Wait -PassThru
        if ($uninstall.ExitCode -ne 0) { throw "El desinstalador devolvio $($uninstall.ExitCode). Carpeta: $stage" }
    }

    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    do {
        $remaining = @(Get-ExistingPaths (@($stage) + $shortcutPaths)) +
            @((Get-ProductRegistryEntries).Location | Where-Object { $_ })
        if ($remaining.Count -eq 0) { break }
        Start-Sleep -Milliseconds 250
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($remaining.Count) { throw "Quedaron rastros de la prueba:`n$($remaining -join "`n")" }
}

Write-Output 'Desinstalacion verificada. Windows retiro los archivos, el registro y los accesos directos de prueba.'

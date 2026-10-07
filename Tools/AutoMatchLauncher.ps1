#requires -Version 5.1
<#
    Pung 자동 대전 실행기 (GUI)

    봇 등급별 수, 판 수, 매치 시간, 게임 속도, 동시 실행 수를 고르고 자동 대전을 띄운다.
    AutoMatch-Launcher.bat 으로 실행한다. 설정은 Saved\AutoMatchLauncher.json 에 저장된다.
    실행 인자는 GDD 7.6 / UPungAutoMatchSubsystem 과 같다.
#>

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$ProjectDir   = Split-Path -Parent $PSScriptRoot
$ProjectFile  = Join-Path $ProjectDir 'Pung.uproject'
$SettingsFile = Join-Path $ProjectDir 'Saved\AutoMatchLauncher.json'

# 정원 8명 - 관전하는 나 1명
$MaxBots = 7

# ------------------------------------------------------------
# 설정
# ------------------------------------------------------------

function Find-EditorExe {
    $candidates = @()
    if ($env:UE_ENGINE_DIR) { $candidates += $env:UE_ENGINE_DIR }
    $candidates += 'C:\Program Files\Epic Games\UE_5.8\Engine'
    $candidates += 'E:\UE_5.8\Engine'
    foreach ($dir in $candidates) {
        $exe = Join-Path $dir 'Binaries\Win64\UnrealEditor.exe'
        if (Test-Path $exe) { return $exe }
    }
    return ''
}

$Settings = [ordered]@{
    Mode        = 'Editor'
    EditorExe   = (Find-EditorExe)
    PackagedExe = ''
    Bots        = @(
        [pscustomobject]@{ Tier = 'Default'; Count = 2 },
        [pscustomobject]@{ Tier = 'Smart';   Count = 2 }
    )
    Matches     = 10
    Duration    = 0
    TimeScale   = 1.0
    Instances   = 1
    Watch       = $false
}

if (Test-Path $SettingsFile) {
    try {
        $loaded = Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($property in $loaded.PSObject.Properties) { $Settings[$property.Name] = $property.Value }
    } catch {
        # 설정 파일이 깨졌으면 기본값으로
    }
}

function Save-Settings {
    $dir = Split-Path -Parent $SettingsFile
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }
    $Settings | ConvertTo-Json -Depth 4 | Set-Content -Path $SettingsFile -Encoding UTF8
}

# ------------------------------------------------------------
# 화면
# ------------------------------------------------------------

$font = New-Object System.Drawing.Font('Malgun Gothic', 9)

$form = New-Object System.Windows.Forms.Form
$form.Text = 'Pung 자동 대전'
$form.Font = $font
$form.ClientSize = New-Object System.Drawing.Size(520, 610)
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.StartPosition = 'CenterScreen'

function New-Label($text, $x, $y, $w = 120) {
    $label = New-Object System.Windows.Forms.Label
    $label.Text = $text
    $label.Location = New-Object System.Drawing.Point($x, $y)
    $label.Size = New-Object System.Drawing.Size($w, 22)
    $label.TextAlign = 'MiddleLeft'
    return $label
}

function New-Number($x, $y, $min, $max, $value, $decimals = 0, $step = 1) {
    $number = New-Object System.Windows.Forms.NumericUpDown
    $number.Location = New-Object System.Drawing.Point($x, $y)
    $number.Size = New-Object System.Drawing.Size(90, 22)
    $number.DecimalPlaces = $decimals
    $number.Increment = $step
    $number.Minimum = $min
    $number.Maximum = $max
    $number.Value = [decimal][Math]::Min([Math]::Max([double]$value, [double]$min), [double]$max)
    return $number
}

# --- 실행 대상 ---
$groupTarget = New-Object System.Windows.Forms.GroupBox
$groupTarget.Text = '실행 대상'
$groupTarget.Location = New-Object System.Drawing.Point(12, 10)
$groupTarget.Size = New-Object System.Drawing.Size(496, 140)
$form.Controls.Add($groupTarget)

$rbEditor = New-Object System.Windows.Forms.RadioButton
$rbEditor.Text = '에디터 빌드 (Build-Editor.bat 으로 빌드한 것)'
$rbEditor.Location = New-Object System.Drawing.Point(12, 20)
$rbEditor.Size = New-Object System.Drawing.Size(470, 22)
$groupTarget.Controls.Add($rbEditor)

$txtEditor = New-Object System.Windows.Forms.TextBox
$txtEditor.Location = New-Object System.Drawing.Point(30, 44)
$txtEditor.Size = New-Object System.Drawing.Size(370, 22)
$txtEditor.Text = $Settings.EditorExe
$groupTarget.Controls.Add($txtEditor)

$btnEditor = New-Object System.Windows.Forms.Button
$btnEditor.Text = '찾기'
$btnEditor.Location = New-Object System.Drawing.Point(408, 43)
$btnEditor.Size = New-Object System.Drawing.Size(74, 24)
$groupTarget.Controls.Add($btnEditor)

$rbPackaged = New-Object System.Windows.Forms.RadioButton
$rbPackaged.Text = '패키징한 exe (Pung.exe)'
$rbPackaged.Location = New-Object System.Drawing.Point(12, 76)
$rbPackaged.Size = New-Object System.Drawing.Size(470, 22)
$groupTarget.Controls.Add($rbPackaged)

$txtPackaged = New-Object System.Windows.Forms.TextBox
$txtPackaged.Location = New-Object System.Drawing.Point(30, 100)
$txtPackaged.Size = New-Object System.Drawing.Size(370, 22)
$txtPackaged.Text = $Settings.PackagedExe
$groupTarget.Controls.Add($txtPackaged)

$btnPackaged = New-Object System.Windows.Forms.Button
$btnPackaged.Text = '찾기'
$btnPackaged.Location = New-Object System.Drawing.Point(408, 99)
$btnPackaged.Size = New-Object System.Drawing.Size(74, 24)
$groupTarget.Controls.Add($btnPackaged)

if ($Settings.Mode -eq 'Packaged') { $rbPackaged.Checked = $true } else { $rbEditor.Checked = $true }

function Select-Exe($textBox, $filter) {
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Filter = $filter
    if ($textBox.Text -and (Test-Path (Split-Path -Parent $textBox.Text))) {
        $dialog.InitialDirectory = Split-Path -Parent $textBox.Text
    }
    if ($dialog.ShowDialog() -eq 'OK') { $textBox.Text = $dialog.FileName }
}
$btnEditor.Add_Click({ Select-Exe $txtEditor 'UnrealEditor.exe|UnrealEditor.exe|exe|*.exe'; $rbEditor.Checked = $true })
$btnPackaged.Add_Click({ Select-Exe $txtPackaged 'exe|*.exe'; $rbPackaged.Checked = $true })

# --- 봇 구성 ---
$groupBots = New-Object System.Windows.Forms.GroupBox
$groupBots.Text = '봇 구성 (등급 이름은 게임 모드 Bot Tiers 키, 기본 등급은 Default)'
$groupBots.Location = New-Object System.Drawing.Point(12, 158)
$groupBots.Size = New-Object System.Drawing.Size(496, 200)
$form.Controls.Add($groupBots)

$grid = New-Object System.Windows.Forms.DataGridView
$grid.Location = New-Object System.Drawing.Point(12, 22)
$grid.Size = New-Object System.Drawing.Size(470, 140)
$grid.AllowUserToAddRows = $true
$grid.AllowUserToDeleteRows = $true
$grid.RowHeadersWidth = 30
$grid.AutoSizeColumnsMode = 'Fill'
$grid.BackgroundColor = [System.Drawing.SystemColors]::Window
[void]$grid.Columns.Add('Tier', '등급')
[void]$grid.Columns.Add('Count', '수')
$grid.Columns[1].FillWeight = 40
foreach ($bot in $Settings.Bots) { [void]$grid.Rows.Add([string]$bot.Tier, [string]$bot.Count) }
$groupBots.Controls.Add($grid)

$lblTotal = New-Label '' 12 168 470
$groupBots.Controls.Add($lblTotal)

# 표에서 봇 목록을 읽는다. 잘못된 값이 있으면 예외.
function Get-Bots {
    $bots = @()
    foreach ($row in $grid.Rows) {
        if ($row.IsNewRow) { continue }
        $tier = "$($row.Cells[0].Value)".Trim()
        $countText = "$($row.Cells[1].Value)".Trim()
        if (-not $tier -and -not $countText) { continue }
        if ($tier -notmatch '^[A-Za-z0-9_]+$') { throw "등급 이름은 영문, 숫자, _ 만 쓸 수 있습니다: '$tier'" }
        $count = 0
        if (-not [int]::TryParse($countText, [ref]$count) -or $count -lt 0) { throw "'$tier' 의 수가 0 이상의 숫자가 아닙니다: '$countText'" }
        $bots += [pscustomobject]@{ Tier = $tier; Count = $count }
    }
    return ,$bots
}

function Update-Total {
    try {
        $bots = Get-Bots
        $total = ($bots | Measure-Object -Property Count -Sum).Sum
        if (-not $total) { $total = 0 }
        $lblTotal.Text = "합계 $total / 최대 $MaxBots 명 (정원 8 - 관전하는 나 1)"
        $lblTotal.ForeColor = if ($total -gt $MaxBots -or $total -eq 0) { [System.Drawing.Color]::Firebrick } else { [System.Drawing.SystemColors]::ControlText }
    } catch {
        $lblTotal.Text = $_.Exception.Message
        $lblTotal.ForeColor = [System.Drawing.Color]::Firebrick
    }
}
$grid.Add_CellValueChanged({ Update-Total })
$grid.Add_RowsRemoved({ Update-Total })
Update-Total

# --- 매치 ---
$groupMatch = New-Object System.Windows.Forms.GroupBox
$groupMatch.Text = '매치'
$groupMatch.Location = New-Object System.Drawing.Point(12, 366)
$groupMatch.Size = New-Object System.Drawing.Size(496, 150)
$form.Controls.Add($groupMatch)

$groupMatch.Controls.Add((New-Label '판 수' 12 22))
$numMatches = New-Number 140 22 0 10000 $Settings.Matches
$groupMatch.Controls.Add($numMatches)
$groupMatch.Controls.Add((New-Label '0 = 끝없이' 240 22 240))

$groupMatch.Controls.Add((New-Label '매치 시간 (초)' 12 52))
$numDuration = New-Number 140 52 0 3600 $Settings.Duration
$groupMatch.Controls.Add($numDuration)
$groupMatch.Controls.Add((New-Label '0 = 게임 모드 값' 240 52 240))

$groupMatch.Controls.Add((New-Label '게임 속도 배율' 12 82))
$numScale = New-Number 140 82 0.5 10 $Settings.TimeScale 1 0.5
$groupMatch.Controls.Add($numScale)
$groupMatch.Controls.Add((New-Label '2 이상은 물리 결과가 달라질 수 있음' 240 82 250))

$groupMatch.Controls.Add((New-Label '동시 실행 수' 12 112))
$numInstances = New-Number 140 112 1 8 $Settings.Instances
$groupMatch.Controls.Add($numInstances)
$groupMatch.Controls.Add((New-Label '기록 폴더는 실행마다 따로 생김' 240 112 250))

# --- 옵션, 버튼 ---
$chkWatch = New-Object System.Windows.Forms.CheckBox
$chkWatch.Text = '화면으로 보기 (창 모드, 아레나 전경 카메라. 끄면 화면 없이 빠르게)'
$chkWatch.Location = New-Object System.Drawing.Point(16, 524)
$chkWatch.Size = New-Object System.Drawing.Size(490, 22)
$chkWatch.Checked = [bool]$Settings.Watch
$form.Controls.Add($chkWatch)

$btnRun = New-Object System.Windows.Forms.Button
$btnRun.Text = '실행'
$btnRun.Location = New-Object System.Drawing.Point(12, 556)
$btnRun.Size = New-Object System.Drawing.Size(150, 34)
$form.Controls.Add($btnRun)
$form.AcceptButton = $btnRun

$btnFolder = New-Object System.Windows.Forms.Button
$btnFolder.Text = '기록 폴더 열기'
$btnFolder.Location = New-Object System.Drawing.Point(172, 556)
$btnFolder.Size = New-Object System.Drawing.Size(150, 34)
$form.Controls.Add($btnFolder)

$lblStatus = New-Label '' 332 556 180
$lblStatus.Size = New-Object System.Drawing.Size(180, 34)
$form.Controls.Add($lblStatus)

# ------------------------------------------------------------
# 동작
# ------------------------------------------------------------

function Get-TelemetryDir {
    if ($rbPackaged.Checked -and $txtPackaged.Text) {
        # 패키징하면 Saved 는 exe 옆 <프로젝트 이름>\Saved
        return Join-Path (Split-Path -Parent $txtPackaged.Text) 'Pung\Saved\Telemetry'
    }
    return Join-Path $ProjectDir 'Saved\Telemetry'
}

function Store-Settings {
    $Settings.Mode        = if ($rbPackaged.Checked) { 'Packaged' } else { 'Editor' }
    $Settings.EditorExe   = $txtEditor.Text
    $Settings.PackagedExe = $txtPackaged.Text
    $Settings.Matches     = [int]$numMatches.Value
    $Settings.Duration    = [int]$numDuration.Value
    $Settings.TimeScale   = [double]$numScale.Value
    $Settings.Instances   = [int]$numInstances.Value
    $Settings.Watch       = $chkWatch.Checked
}

$btnRun.Add_Click({
    try {
        $bots = Get-Bots
        $total = ($bots | Measure-Object -Property Count -Sum).Sum
        if (-not $total) { throw '봇이 0명입니다.' }
        if ($total -gt $MaxBots) { throw "봇은 최대 $MaxBots 명입니다. 지금 $total 명." }
        $spec = ($bots | Where-Object { $_.Count -gt 0 } | ForEach-Object { "$($_.Tier):$($_.Count)" }) -join ','

        $invariant = [System.Globalization.CultureInfo]::InvariantCulture
        $arguments = @(
            '-PungAutoMatch',
            "-PungBots=$spec",
            "-PungMatches=$([int]$numMatches.Value)",
            "-PungMatchDuration=$([int]$numDuration.Value)",
            ('-PungTimeScale=' + ([double]$numScale.Value).ToString($invariant)),
            '-nosteam', '-unattended', '-log'
        )
        if ($chkWatch.Checked) {
            $arguments += @('-windowed', '-ResX=960', '-ResY=540')
        } else {
            $arguments += @('-nullrhi', '-nosound')
        }

        if ($rbPackaged.Checked) {
            $exe = $txtPackaged.Text
            if (-not $exe -or -not (Test-Path $exe)) { throw '패키징한 exe 를 찾을 수 없습니다. [찾기] 로 Pung.exe 를 고르세요.' }
        } else {
            $exe = $txtEditor.Text
            if (-not $exe -or -not (Test-Path $exe)) { throw 'UnrealEditor.exe 를 찾을 수 없습니다. [찾기] 로 엔진의 Binaries\Win64\UnrealEditor.exe 를 고르세요.' }
            if (-not (Test-Path $ProjectFile)) { throw "프로젝트를 찾을 수 없습니다: $ProjectFile" }
            $arguments = @("`"$ProjectFile`"", '-game') + $arguments
        }

        $Settings.Bots = $bots
        Store-Settings
        Save-Settings

        $instances = [int]$numInstances.Value
        for ($i = 0; $i -lt $instances; $i++) {
            Start-Process -FilePath $exe -ArgumentList $arguments | Out-Null
            if ($i -lt $instances - 1) { Start-Sleep -Milliseconds 800 }
        }
        $lblStatus.Text = "$instances 개 실행: $spec"
    } catch {
        [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Pung 자동 대전', 'OK', 'Warning') | Out-Null
    }
})

$btnFolder.Add_Click({
    $dir = Get-TelemetryDir
    if (Test-Path $dir) {
        Start-Process explorer.exe $dir
    } else {
        [System.Windows.Forms.MessageBox]::Show("아직 기록이 없습니다.`n$dir", 'Pung 자동 대전', 'OK', 'Information') | Out-Null
    }
})

$form.Add_FormClosing({
    try {
        $Settings.Bots = Get-Bots
    } catch {
        # 표에 잘못된 값이 있으면 봇 목록은 저장하지 않는다
    }
    Store-Settings
    Save-Settings
})

[void]$form.ShowDialog()

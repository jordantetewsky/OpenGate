function Build-UnboundIcons {
    param([string]$repoRoot)
    $Source = Join-Path $repoRoot 'src\assets\unbound-symbol.svg'
    $Destination = Join-Path $repoRoot 'src\assets'
    $Icon = Join-Path $repoRoot 'src\platforms\windows\resources\unbound.ico'
    Add-Type -AssemblyName System.Drawing
    [xml]$svg = Get-Content -LiteralPath $Source -Raw
    $group = $svg.DocumentElement.g
    $color = [Drawing.ColorTranslator]::FromHtml($group.fill)
    $paths = @()
    try {
        # The master uses M/L/Q/C/Z only. Reject unsupported geometry rather than
        # silently building a different icon; no SVG runtime dependency is added.
        foreach ($element in $group.path) {
            $path = [Drawing.Drawing2D.GraphicsPath]::new([Drawing.Drawing2D.FillMode]::Alternate)
            $paths += $path
            $tokens = @([regex]::Matches($element.d, '[A-Za-z]|[-+]?(?:\d*\.)?\d+') | ForEach-Object { $_.Value })
            $x = [float]0; $y = [float]0; $i = 0
            while ($i -lt $tokens.Count) {
                $command = $tokens[$i++]
                if ($command -eq 'Z') { $path.CloseFigure(); continue }
                $count = switch ($command) { 'M' {2} 'L' {2} 'Q' {4} 'C' {6} default { throw "Unsupported SVG command: $command" } }
                $values = @()
                for ($j=0; $j -lt $count; $j++) {
                    $values += [float]::Parse($tokens[$i++], [Globalization.CultureInfo]::InvariantCulture)
                }
                if ($command -eq 'M') { $path.StartFigure() }
                elseif ($command -eq 'L') { $path.AddLine($x,$y,$values[0],$values[1]) }
                elseif ($command -eq 'C') { $path.AddBezier($x,$y,$values[0],$values[1],$values[2],$values[3],$values[4],$values[5]) }
                else {
                    # Convert the quadratic curve to the equivalent cubic.
                    $path.AddBezier($x,$y,($x+2*($values[0]-$x)/3),($y+2*($values[1]-$y)/3),
                        ($values[2]+2*($values[0]-$values[2])/3),($values[3]+2*($values[1]-$values[3])/3),$values[2],$values[3])
                }
                $x=$values[$count-2]; $y=$values[$count-1]
            }
        }
        $sizes = @(16,20,24,32,40,48,64,96,128,256)
        $frames = @()
        foreach ($size in $sizes) {
            $bitmap = [Drawing.Bitmap]::new($size,$size,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $draw = [Drawing.Graphics]::FromImage($bitmap)
            $brush = [Drawing.SolidBrush]::new($color)
            $stream = [IO.MemoryStream]::new()
            try {
                $draw.Clear([Drawing.Color]::Transparent)
                $draw.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
                $draw.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
                $scale = $size / 20.0
                for ($index=0; $index -lt $paths.Count; $index++) {
                    $points = $paths[$index].PathPoints
                    if ($index -eq 0) { $xs = @(1.75,3.25,14.75,16.25); $ys = @(2.75,4.25,15.75,17.25) }
                    else { $xs = @(11.75,13.25,17.75,19.25); $ys = @(6.25,7.75,12.25,13.75) }
                    # Small icons get integer-aligned straight edges and the same
                    # integer stroke width on all four sides. Curves retain AA.
                    $thickness = [Math]::Max(1,[Math]::Round(1.5*$scale))
                    foreach ($pointIndex in 0..($points.Count-1)) {
                        $coords = @([double]$points[$pointIndex].X,[double]$points[$pointIndex].Y)
                        for ($axis=0; $axis -lt 2; $axis++) {
                            $value = $coords[$axis]*$scale
                            if ($size -le 64) {
                                $knots = if ($axis -eq 0) { $xs } else { $ys }
                                $low = [Math]::Round($knots[0]*$scale)
                                $high = [Math]::Round($knots[3]*$scale)
                                $aligned = @($low,($low+$thickness),($high-$thickness),$high)
                                $segment = 0
                                while ($segment -lt 2 -and $coords[$axis] -gt $knots[$segment+1]) { $segment++ }
                                $fraction = ($coords[$axis]-$knots[$segment])/($knots[$segment+1]-$knots[$segment])
                                $value = $aligned[$segment]+$fraction*($aligned[$segment+1]-$aligned[$segment])
                            }
                            $coords[$axis] = $value
                        }
                        $points[$pointIndex] = [Drawing.PointF]::new($coords[0],$coords[1])
                    }
                    $renderPath = [Drawing.Drawing2D.GraphicsPath]::new($points,$paths[$index].PathTypes,[Drawing.Drawing2D.FillMode]::Alternate)
                    try { $draw.FillPath($brush,$renderPath) } finally { $renderPath.Dispose() }
                }
                $bitmap.Save($stream,[Drawing.Imaging.ImageFormat]::Png)
                $bytes = $stream.ToArray()
                [IO.File]::WriteAllBytes((Join-Path $Destination "unbound-$size.png"),$bytes)
                $frames += ,$bytes
            } finally {
                $stream.Dispose(); $brush.Dispose(); $draw.Dispose(); $bitmap.Dispose()
            }
        }
        # Modern Windows ICO: PNG-compressed 32-bit RGBA frames, 256 encoded as 0.
        $output = [IO.File]::Create($Icon)
        $writer = [IO.BinaryWriter]::new($output)
        try {
            $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]$sizes.Count)
            $offset = 6 + 16*$sizes.Count
            for ($i=0; $i -lt $sizes.Count; $i++) {
                $dimension = if ($sizes[$i] -eq 256) { 0 } else { $sizes[$i] }
                $writer.Write([byte]$dimension); $writer.Write([byte]$dimension)
                $writer.Write([byte]0); $writer.Write([byte]0)
                $writer.Write([uint16]1); $writer.Write([uint16]32)
                $writer.Write([uint32]$frames[$i].Length); $writer.Write([uint32]$offset)
                $offset += $frames[$i].Length
            }
            foreach ($frame in $frames) { $writer.Write([byte[]]$frame) }
        } finally { $writer.Dispose(); $output.Dispose() }
        Write-Host "Created symbol PNG variants in $Destination and Windows icon at $Icon"
    } finally { foreach ($path in $paths) { $path.Dispose() } }
}

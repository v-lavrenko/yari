#!/usr/local/bin/gawk -f

BEGIN { }

{
    ++N;
    X = $1; SX += X; SX2 += X*X; 
    Y = $2; SY += Y; SY2 += Y*Y; SXY += X*Y;
    D = X-Y;
    SAE += (X>Y) ? (X-Y) : (Y-X);
    SSE += (X-Y) * (X-Y);
}

END {
    EX = SX/N; VX = SX2/N - EX*EX; EXY = SXY / N;
    EY = SY/N; VY = SY2/N - EY*EY;
    MSE = SSE/N;
    MAE = SAE/N;
    COV = EXY - EX*EY;
    CC = COV / sqrt (VX  * VY);
    C2 = CC*CC;
    R2 = 1 - SSE / (N*VX);
    
    printf "%9s: %9.9s %9.9s %9.9s %9.9s %9.9s %9.9s\n", "num", "MAE", "MSE", "CC", "C2", "R2", "COV";
    printf "%9d: %9.4f %9.4f %9.4f %9.4f %9.4f %9.4f\n",     N,  MAE,   MSE,   CC,   C2,   R2,   COV;
    
}

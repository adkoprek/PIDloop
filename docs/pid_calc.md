# PID

## Wiki Formula for offset

$$
(K_p+K_i\Delta_t+\frac{K_d}{\Delta_t})\epsilon[n]+(-K_p-\frac{2K_d}{\Delta_t})\epsilon[n-1]+\frac{K_d}{\Delta_t}\epsilon[n-2]
$$

Keep in mind that the parameters $K_i, K_d$ can be also expressed otherwise using physical units that are useful in production.

$$
K_i=\frac{K_p}{T_i}\\
K_d=K_p*T_d
$$

$T_i, T_d$ are expressed in seconds. With $T_i$ you can control the integration time or how long the controler will tolerate the output being consistently above the set point. With $T_d$ you regulate the derivativ etime or is the time constant with which the controler will atempt to approach the set point.

## Metzger Code Formula for offset

In the original `reg2d` appliction $T_i$ and $T_d$ are in the expressed in the unit $0.01s$ and the $f$ freqency is in $Hz$ 

This is the formula for the code

$$
K_p(\epsilon[n]-\epsilon[n-1]+\frac{\epsilon[n-1]}{f*T_i}+(\epsilon[n]-2\epsilon[n-1]+\epsilon[n-2])*\frac{T_d*f}{10})
$$

Now we express the above equasion as a sum

$$
K_p\epsilon[n]-K_p\epsilon[n-1]+\frac{K_p*\epsilon[n-1]}{f*T_i}+\frac{K_p\epsilon[n]*T_d*f}{10}-\frac{2*\epsilon[n-1]*T_d*f}{10}+\frac{K_p*\epsilon[n-2]*T_d*f}{10}
$$

Now we can group the expressions by their error and factor it out

$$
(K_p+\frac{K_p*T_d*f}{10})\epsilon[n]+(-K_p+\frac{K_p}{f*T_i}-\frac{K_p*2*T_d*f}{10})\epsilon[n-1]+\frac{K_p*T_d*f}{10}*\epsilon[n-2]
$$

Now we can replace the expressions $K_p*T_d$ for $K_d$, $K_p*T_i$ for $T_i$ and $\frac{1}{f}$ for $\Delta_t$

$$
(K_p+\frac{K_d}{10*\Delta_t})\epsilon[n]+(-K_p+K_i\Delta_t-\frac{2*K_d}{10*\Delta_t})\epsilon[n-1]+\frac{K_d}{10*\Delta_t}\epsilon[n-2]
$$

And here we have it nearly the same formula as on Wikipedia. The only two differences are that the intergral term is now
in the expression with $\epsilon[n-1]$ and that every $K_i, K_d$ is divided by 10. We don't know for sure the intentions
for that of the original Author but one hypothesis is that he didn't want to use a decimal slider in `reg2d`.

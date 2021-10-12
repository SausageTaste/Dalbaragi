$$
\begin{pmatrix}
    \cos \theta_y  & 0 & \sin \theta_y \\
    0              & 1              & 0      \\
    -\sin \theta_y & 0              & cos \theta_y      \\
\end{pmatrix} \times \begin{pmatrix}
    1              & 0              & 0      \\
    0              & \cos \theta_x  & -\sin \theta_x      \\
    0              & \sin \theta_x  & \cos \theta_x      \\
\end{pmatrix} \times \begin{pmatrix}
    \cos \theta_z  & -\sin \theta_z & 0      \\
    \sin \theta_z  & \cos \theta_z  & 0      \\
    0              & 0 & 1      \\
\end{pmatrix}
$$

Full rotation matrix of euler YXZ angles

$$
\begin{pmatrix}
    \sin \theta_x \sin \theta_y \sin \theta_z + \cos \theta_y \cos \theta_z
    &
    \sin \theta_x \sin \theta_y \cos \theta_z - \cos \theta_y \sin \theta_z
    &
    \cos \theta_x \sin \theta_y

    \\

    \cos \theta_x \sin \theta_z
    &
    \cos \theta_x \cos \theta_z
    &
    -\sin \theta_x

    \\

    \sin \theta_x \cos \theta_y \sin \theta_z - \sin \theta_y \cos \theta_z
    &
    \sin \theta_x \cos \theta_y \cos \theta_z + \sin \theta_y \sin \theta_z
    &
    \cos \theta_x \cos \theta_y
\end{pmatrix} = \begin{pmatrix}
    M_{00} & M_{10} & M_{20} \\
    M_{01} & M_{11} & M_{21} \\
    M_{02} & M_{12} & M_{22} \\
\end{pmatrix}
$$

Simplified version

$$
\begin{pmatrix}
    s_x s_y s_z + c_y c_z
    &
    s_x s_y c_z - c_y s_z
    &
    c_x s_y

    \\

    c_x s_z
    &
    c_x c_z
    &
    -s_x

    \\

    s_x c_y s_z - s_y c_z
    &
    s_x c_y c_z + s_y s_z
    &
    c_x c_y
\end{pmatrix}
$$

Euler angles $E$ can be calculated like followings

$$ E_x = atan2(\sin \theta_x, \sqrt{\cos^2 \theta_x \sin^2 \theta_z + \cos^2 \theta_x \cos^2 \theta_z}) $$
$$ E_y = atan2(\cos \theta_x \sin \theta_y, \cos \theta_x \cos \theta_y) $$
$$ E_z = atan2(\cos \theta_x \sin \theta_z, \cos \theta_x \cos \theta_z) $$

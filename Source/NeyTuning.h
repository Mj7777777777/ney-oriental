// Table de correction de justesse du modele de flute (en cents), mesuree hors-ligne.
// Lignes : notes MIDI 30..100 par pas de 2. Colonnes : Brightness 0, 0.25, 0.5, 0.75, 1.
#pragma once
#include <algorithm>
namespace ney
{
inline float tuningCorrectionCents (float note, float brightness) noexcept
{
    static const float t[36][5] = {
        {   -4.3f,   -6.7f,   -8.1f,   -9.1f,   -9.8f },
        {   -3.8f,   -6.7f,   -8.5f,   -9.7f,  -10.5f },
        {   -2.5f,   -6.4f,   -8.6f,  -10.1f,  -11.1f },
        {   -0.7f,   -5.1f,   -8.0f,  -10.0f,  -11.4f },
        {    1.4f,   -3.2f,   -6.4f,   -8.8f,  -10.6f },
        {    3.5f,   -1.0f,   -4.3f,   -6.7f,   -8.6f },
        {    5.6f,    1.2f,   -2.0f,   -4.4f,   -6.3f },
        {    7.5f,    3.2f,    0.1f,   -2.2f,   -4.0f },
        {    9.3f,    5.1f,    2.1f,   -0.1f,   -1.9f },
        {   10.9f,    6.9f,    4.0f,    1.8f,    0.2f },
        {   12.4f,    8.5f,    5.7f,    3.6f,    2.0f },
        {   13.7f,   10.0f,    7.3f,    5.3f,    3.7f },
        {   14.9f,   11.3f,    8.7f,    6.8f,    5.3f },
        {   16.0f,   12.5f,   10.0f,    8.1f,    6.6f },
        {   16.9f,   13.6f,   11.1f,    9.3f,    7.9f },
        {   17.8f,   14.5f,   12.2f,   10.4f,    9.0f },
        {   18.5f,   15.4f,   13.1f,   11.4f,   10.0f },
        {   19.2f,   16.1f,   13.9f,   12.2f,   10.9f },
        {   19.8f,   16.8f,   14.6f,   13.0f,   11.8f },
        {   20.3f,   17.4f,   15.3f,   13.7f,   12.5f },
        {   20.8f,   18.0f,   15.9f,   14.3f,   13.1f },
        {   21.2f,   18.5f,   16.4f,   14.9f,   13.7f },
        {   21.6f,   18.9f,   16.9f,   15.4f,   14.3f },
        {   22.0f,   19.3f,   17.3f,   15.9f,   14.7f },
        {   22.3f,   19.7f,   17.7f,   16.3f,   15.2f },
        {   22.6f,   20.0f,   18.1f,   16.7f,   15.6f },
        {   22.9f,   20.3f,   18.4f,   17.0f,   15.9f },
        {   23.1f,   20.6f,   18.7f,   17.4f,   16.3f },
        {   23.4f,   20.8f,   19.0f,   17.6f,   16.6f },
        {   23.6f,   21.1f,   19.3f,   17.9f,   16.9f },
        {   23.8f,   21.3f,   19.5f,   18.1f,   17.2f },
        {   24.0f,   21.6f,   19.7f,   18.4f,   17.5f },
        {   24.2f,   21.8f,   20.0f,   18.6f,   17.5f },
        {   24.5f,   22.1f,   20.3f,   19.0f,   17.9f },
        {   24.7f,   22.2f,   20.4f,   19.1f,   18.3f },
        {   24.8f,   22.5f,   20.6f,   19.3f,   18.4f }
    };
    const float x = std::max (0.0f, std::min (34.999f, (note - 30.0f) * 0.5f));
    const float y = std::max (0.0f, std::min (3.999f, brightness * 4.0f));
    const int i = (int) x, j = (int) y;
    const float fx = x - (float) i, fy = y - (float) j;
    const float a = t[i][j] + (t[i][j + 1] - t[i][j]) * fy;
    const float b = t[i + 1][j] + (t[i + 1][j + 1] - t[i + 1][j]) * fy;
    return a + (b - a) * fx;
}
} // namespace ney

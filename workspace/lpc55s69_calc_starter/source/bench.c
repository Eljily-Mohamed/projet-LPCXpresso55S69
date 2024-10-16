#include "sysdep.h"  // Inclure les déclarations de fonctions de sysdep.c
#include "calc.h"    // Pour la structure Calc et les fonctions associées

// Fonction pour dessiner les données d'accélération sur un graphique
header* draw_x_y_z(Calc *cc, header *hd) {
    // Utiliser la fonction maccel pour obtenir les données d'accélération
    header *accel_data = maccel(cc, hd);

    if (accel_data == NULL) {
        // Gérer l'erreur si aucune donnée d'accélération n'est récupérée
        return NULL;
    }

    // Extraire les valeurs X, Y, Z du tableau de la matrice
    real *m = matrixof(accel_data);

    real accel_x = m[0];  // Accélération selon l'axe X
    real accel_y = m[1];  // Accélération selon l'axe Y
    real accel_z = m[2];  // Accélération selon l'axe Z

    // Configuration de l'affichage du graphique
    gsubplot(1, 1, 1);  // Dessiner un seul sous-graphe (1x1)
    gsetplot(-1, 3, -2, 2, 0, 0);  // Définir les limites des axes : X entre -1 et 3, Y entre -2 et 2

    // Définir la grille des axes X et Y
    header *xticks = new_matrix(cc,1,5,"");  // Ticks pour l'axe X : [-1, 0, 1, 2, 3]
    header *yticks = new_matrix(cc,1,5,"");  // Ticks pour l'axe Y : [-2, -1, 0, 1, 2]
    real *xv = matrixof(xticks);
    real *yv = matrixof(yticks);

    for (int i = 0; i < 5; i++) {
        xv[i] = -1 + i;  // Valeurs de -1 à 3 pour X
        yv[i] = -2 + i;  // Valeurs de -2 à 2 pour Y
    }

    gsetxgrid(xticks, 1, 16);  // Tracer la grille de l'axe X
    gsetygrid(yticks, 1, 16);  // Tracer la grille de l'axe Y

    // Créer des headers pour stocker les valeurs X et Y à tracer
    header *hdx = new_matrix(cc,1, 3, "");  // Pour les valeurs X à tracer
    header *hdy = new_matrix(cc,1,3,"");  // Pour les valeurs Y à tracer
    real *vx = matrixof(hdx);
    real *vy = matrixof(hdy);

    // Stocker les données d'accélération dans les headers
    vx[0] = accel_x;  // Valeur X
    vx[1] = accel_y;  // Valeur Y
    vx[2] = accel_z;  // Valeur Z

    // Tracer les données sur le graphique
    gplot(cc, hdx, hdy);

    // Ajouter des labels et un titre au graphique
    mxlabel("x");
    mylabel("y");
    mtitle("Acceleration Data");

    return accel_data;  // Retourner les données d'accélération pour un traitement ultérieur si nécessaire
}

# MOS 2.2 - Informatique Graphique

## Table of contents

I. [Ray Tracing](#raytracing)

1. [Ray Tracing primitif](#raytracingprimitif)
2. [Ajout de l'intensité lumineuse](#intensitelumineuse)
3. [Création d'une scène](#scenecreation)
4. [Correction Gamma](#correctiongamma)
5. [Ombres portées](#ombresportees)
6. [Surfaces miroir](#surfacesmiroir)
7. [Surfaces transparents](#surfacestransparentes)
8. [Transmission de Fresnel](#fresnel)
9. [Eclairage indirect](#eclairageindirect)

   a) [Generation de nombre aléatoire](#randomnumber)

   b) [Résultat de l'éclairage indirect](#resulteclairageindirect)

10. [Parallélisation des calculs](#parallelisation)
11. [Crénelage](#crenelage)

II. [Feedback sur le MOS](#feedback)

Enseignant : Nicolas Bonneel

Etudiant : Julien Verdun

Date : 06/01/2021

## Ray Tracing <a name="raytracing"></a>

Le ray tracing est une technique de calcul d'optique par ordinateur, utilisée pour le rendu en synthèse d'image ou pour des études de systèmes optiques

On décrit dans cette partie l'implémentation d'un algorithme de **Ray Tracing**. Les différentes étapes permettant de complexifier notre algorithme sont expliquées.

Le fichier `raytracer.cpp` contient les classes **C++** et le **main** nécessaires pour créer les images présentées plus bas.

### Ray Tracing primitif <a name="raytracingprimitif"></a>

Le premier Raytracer implémenté est très rudimentaire.

On représente une **source de lumière blanche** par une source omnidirectionnelle placée en un point L.
Une caméra est placée en un point C. Elle est composée d'une grille de pixels placée à une distance Z de C et de taille (W,H), soit W\*H pixels au total.

Dans un premier temps, on place une sphère devant la caméra. C'est une sphère de centre O et de rayon R.

La démarche est la suivante. On trace des demi-droites partant du centre de la caméra C et dirigé par un vecteur **u**. Une demi-droite traverse la grille en un pixel, il y a donc W\*H demi-droites, passant par C et dirigée par le vecteur unitaire :

**u** = ( j-W/2, i-H/2, -W/(2.tan(&alpha;/2)))

où (i,j) sont les coordonnées du pixel sur l'écran de la caméra.

Pour chaque demi-droite, on calcule l'intersection avec la sphère.
On résoud :

t<sup>2</sup> + 2.t.&lt;**u**,**CO**&gt; + ||**CO**||<sup>2</sup> - R<sup>2</sup> = 0

Si l'intersection existe (**au moins une solution t positive**), on représente le pixel correspondant en blanc, sinon on le laisse noir.

Ainsi on obtient la Figure représentée ci-dessous. La sphère est bien représentée en blanc et le reste en noir.

**Remarque :** avec cette méthode assez primitive, il n'est pas possible d'observer le volume de la sphère, ainsi que les ombres créées par la lumière.

![raytracer_intensite_nulle](Figures/raytracer_intensite_nulle.png)

Les classes implémentées sont les suivantes :

- une classe **Vector** : elle contient les coordonnées du vecteur et différentes méthodes pour surcharger les opérations d'addition, soustraction, produit scalaire et produit par une constante ainsi que de calcul de la norme
- une classe **Sphere** : elle contient le centre et le rayon de la sphère
- une classe **Ray** : elle contient un point du rayon et son vecteur directeur.

### Ajout de l'intensité lumineuse <a name="intensitelumineuse"></a>

On change ici la couleur attribuée à un pixel de la caméra afin d'obtenir du contraste et de discerner les volumes des objets.

Au lieu de donner une couleur blanche à un pixel dont un rayon serait entré en collision avec un objet, on donne une couleur dont l'intensité est variable.

On calcule l'intensité de la lumière en un point P d'un objet selon la formule :

I<sub>p</sub> = I/(4.&pi;.||**LP**||<sup>2</sup>) . .&lt;**N**,**PL**/||**PL**||&gt; . &rho; / &pi;

avec :

- I l'intensité de la source lumineuse au point L
- **N** la normale à l'objet (la sphère) au point P (vecteur unitaire)
- &rho; l'albédo de la sphère, i-e la fraction de la lumière que la sphère réfléchit.

Ainsi on obtient la Figure représentée ci-dessous. La sphère, représentée avec un albedo rouge, est bien représentée en rouge et le reste en noir. On peut cette fois mieux apprécier le volume de la sphère et on peut clairement identifier la localisation de la source de lumière grâce aux différences d'intensiter lumineuses (les ombres sont mieux représentés). Ce raytracer est plus performant que la version précédente.

![raytracer_avec_intensite](Figures/raytracer_avec_intensite.png)

### Création d'une scène <a name="scenecreation"></a>

On désire à présent **complexifier la scène observée**. Jusqu'à présent, seul une sphère était placée devant la caméra.

On implémente une classe **Scene** contenant un vecteur d'objets, qui contiendra les objets (par exemple des sphères) placés devant la caméra.

Afin de bien représenter les objets les un par rapport aux autres, il convient de pouvoir déterminer leur profondeur dans l'espace. Ainsi pour réaliser cette modification, une méthode est implémentée afin de vérifier pour un rayon donné, les objets de la scène qui intercepte ce rayon, et de ne garder que l'objet le plus proche de la caméra (l'objet qui va cacher les objets derrière lui).

On réalise un essai en créant une scène composée de :

- trois sphères, de gauche à droite bleue, blanche et rouge
- 2 murs à gauche et à droite respectivement bleu et rouge
- 2 murs en face de la caméra et derrière la caméra (invisible donc) respectivement vert et magenta
- 1 mur au sol de couleur blanche.

Les murs sont représentés par des sphéres de très grand rayon et de centre très éloigné.
Le résultat obtenu est présenté sur la Figure ci-dessous :

![raytracer_plusieurs_spheres](Figures/raytracer_plusieurs_spheres.png)

**Remarque** : Le résultat est assez satisfaisant, toutefois certaines améliorations sont possibles :

- les ombres des sphères ne sont pas projetées sur les murs alentours
- la luminosité de la lumière sur les murs est très faible, leur couleur n'est pas très bien perceptible.

### Correction Gamma <a name="correctiongamma"></a>

On se propose ici d'améliorer le contraste des objets. En effet, sur la dernière scène obtenue, nous avons relevé la faible luminosité des murs en arrière plan.

La **correction Gamma** permet d'améliorer le contraste en réalisant une opération simple sur l'intensité lumineuse calculée de chaque pixel :

I = I<sub>calculee</sub><sup>1/&gamma;</sup>

avec &gamma; = 2,2 et 1/&gamma; = 0,45.

La correction Gamma permet d'obtenir le résultat ci-dessous. La luminosité des pixels représentant les murs est plus importante. Le rendu est meilleur.

![raytracer_correction_gamma](Figures/raytracer_correction_gamma.png)

### Ombres portées <a name="ombresportees"></a>

On souhaite encore améliorer le rendu en ajoutant les **ombres portées**, c'est-à-dire les ombres des objets projetées sur les surfaces.

La stratégie est la suivante. On envoie des rayons dans toutes les directions (vers chaque pixel de l'écran). Pour un rayon donné, on trouve l'intersection avec un objet de la scène puis on regarde si le rayon entre cette intersection et la source de lumière coupe un autre objet. Si tel est le cas, on rend le pixel d'intersection plus sombre.

On obtient un premier résultat présenté sur la Figure ci-dessous. On remarque que les ombres sont bien présentes, toutefois, l'image est très bruitée.

![raytracer_shadow_and_noise](Figures/raytracer_shadow_and_noise.png)

Le bruit s'explique par les incertitudes de calcul dû à la précision numérique de la machine. Ainsi, le point d'intersection se retrouve dans la surface au lieu d'être sur la surface. Pour éviter cela, on déplace le point d'intersection d'une petite quantité &epsilon; vers l'extérieur de l'objet.

Avec cette légère modification, on obtient le résultat présenté ci-dessous, le bruit n'est plus présent et les ombres sont bien visibles.

![raytracer_avec_shadow](Figures/raytracer_avec_shadow.png)

### Surfaces miroir <a name="surfacesmiroir"></a>

Les surfaces représentées jusqu'à présent sont des surfaces opaques qui possèdent un albédo. On se propose ici de représenter un autre type de surface, les **surfaces miroir**.

Un rayon lumineux se reflète sur une telle surface avec un angle de réflexion par rapport à la normale de la surface égal à l'angle d'incidence. Les réflexions peuvent être multiples dans le cas de plusieurs surfaces miroir.

On implémente ces types de surfaces en créant une fonction getColor récursive qui permet de donner la couleur entre l'intersection d'un rayon et d'une surface opaque, ou le cas écheant entre le rayon réfléchi et une surface miroir.

Avec cette propriété, notre **raytracer** devient un **pathtracer**.

Le rayon incident **i** est réfléchi par la surface miroir en un rayon **r** tel que :

**r** = **i** - 2<**i**,**N**>**N**

On remplace la sphère centrale des scènes précédentes par une surface miroir. On obtient le résultat ci-dessous.

![raytracer_mirror_noise](Figures/raytracer_mirror_noise.png)

On rencontre le même problème que précédemment, du bruit est présent sur la surface dû aux erreurs numériques. Le résultat après correction est le suivant.

![raytracer_mirror](Figures/raytracer_mirror.png)

La sphère centrale permet bien de refléter le mur derrière l'écran, le mur magenta et les sphères et murs environnants.

### Surfaces transparents <a name="surfacestransparentes"></a>

On se propose ici de représenter un autre type de surface, les **surfaces transparentes**.

On fait l'hypothèse qu'une surface transparente réfléchie l'intégralité d'un rayon ou transmet l'intégralité du rayon, on ne tient pas compte de la **transmission de Fresnel**.

On utilise la **loi de Snell-Descartes**, pour une surface de normale **N** séparant deux milieux d'indices respectifs n<sub>1</sub> et n<sub>2</sub>, un rayon incident dans le milieu d'indice n<sub>1</sub> avec un angle &theta;<sub>i</sub> est transmis dans le milieu d'indice n<sub>2</sub> avec un angle &theta;<sub>t</sub> tel que :

n<sub>1</sub> sin(&theta;<sub>i</sub>) = n<sub>2</sub> sin(&theta;<sub>t</sub>)

Grâce à cette loi, on peut exprimer les composantes tangentielle et normale du vecteur unitaire directeur du rayon transmis **T** :

**T<sub>N</sub>** = - &radic;(1 - (n<sub>1</sub>/n<sub>2</sub>)<sup>2</sup>(1-<**i**,**N**><sup>2</sup>)) **N**

**T<sub>T</sub>** = n<sub>1</sub>/n<sub>2</sub> (**i** - <**i**,**N**>**N**)

Connaissant la direction du rayon tranmis par la surface transparente, il est alors possible de connaître la couleure du pixel à afficher en cherchant l'intersection de ce rayon avec le reste de la scène.

On modifie la sphère de droite (sphère rouge) en une surface transparente. On obtient le résultat présenté sur la Figure ci-dessous. La sphère est bien transparente et laisse apparaître avec une inversion de la direction les murs de droite et du fond.

![raytracer_transparent](Figures/raytracer_transparent.png)

Une autre représentation est visible ci-dessous en remplaçant le sol par un miroir.
On calcule le temps nécessaire pour créer cette scène. L'algorithme donne les performances suivantes :

- 0.918 seconde pour générer l'image sans le sol en miroir
- 1.137 secondes pour générer l'image avec le sol en miroir.

![raytracer_transparent_sol_mirroir](Figures/raytracer_transparent_sol_mirroir.png)

### Transmission de Fresnel <a name="fresnel"></a>

Lors de la transmission d'un rayon par une surface transparente, une partie du rayon est en réalité réfléchie par la surface, selon les lois de Fresnel.

Dans l'approximation de Schlick, le rayon incident se divise en un rayon réfléchi **R** et un rayon transmis **T** selon les formules suivantes :

R = k<sub>0</sub> + (1-k<sub>0</sub>)(1-|<**N**,**i**>|)<sup>5</sup>

T = 1 - R

avec k<sub>0</sub> = (n<sub>1</sub>-n<sub>2</sub>)<sup>2</sup>/(n<sub>1</sub>+n<sub>2</sub>)<sup>2</sup>

Pour implémenter la transmission de Fresnel, deux stratégies ont été testées.

La **première stratégie** consiste à calculer la valeur d'un pixel transparent en faisant une moyenne pondérée par les coefficients R et T du rayon réfléchi et du rayon réfracté. L'inconvénient de cette méthode est son temps de calcul. En effet, la méthode de calcul de la couleur étant récursive, l'appeler 2 fois pour calculer la valeur d'un pixel transparent conduit à fait 2<sup>N</sup> calcul pour N rebonds, la complexité devient très vite importante.

Les résultats de cette méthode sont très concluants puisqu'en moins de 2 secondes, les deux images ci-dessous sont générées avec un rendu proche de la réalité.

![raytracer_fresnel_gourmand](Figures/raytracer_fresnel_gourmand.png)

![raytracer_fresnel_gourmand_sol_mirror](Figures/raytracer_fresnel_gourmand_sol_mirror.png)

La **deuxième stratégie** consiste à calculer la valeur d'un pixel transparent en faisant une moyenne de plusieurs chemins complets pour lesquels à chaque intersection avec une surface transparente, on choisit aléatoirement d'émettre un rayon réfléchi ou réfracté, en générant un nombre entre 0 et 1, et en le comparant au coefficient R. Si le nombre généré est inférieur à R, le rayon sera réfléchi, sinon il sera réfracté.

Les résultats de cette méthode semble un peu moins performant. en effet, en générant et moyennant la valeur du pixel pour 30 rayons complets, on obtient le résultat ci-dessous. La qualité de la transparence est moins importante qu'avec la première méthode, on observe un léger bruit. De plus, le temps pour générer l'image est plus grand avec environ 6 secondes pour 10 rayons et 20 secondes pour 30 rayons.

![raytracer_fresnel_30_tirages](Figures/raytracer_fresnel_30_tirages.png)

### Eclairage indirect <a name="eclairageindirect"></a>

Avec le pathtracer actuel, les surfaces avec un éclairage rasant possèdent une faible intensité lumineuse.

En réalité, les surfaces environnentes réflechissent une partie de la lumière pour éclairer ces surfaces "rasantes" et ainsi augmenter la quantité de lumière observée. Ainsi les surfaces se comportent comme des sources de lumière secondaires. On parle d'**éclairage indirect**.

L'idée est la suivante. On considère que chaque rayon qui arrive sur un pixel de la caméra est la combinaison de la lumière réfléchie par la source lumineuse ponctuelle sur la surface intersectée, plus une quantité de lumière qui provient de la réflection d'autres rayons sur des surfaces.

La première quantité est celle que l'on calcule déjà.

La deuxième se calcule avec l'intégrale sur la demi-sphère S<sup>+</sup> de la quantité :

&int; f(**w<sub>i</sub>** , **w<sub>o</sub>**) . L(**w<sub>i</sub>**) . <**N**,**w<sub>i</sub>**> . d<sub>w<sub>i</sub></sub>

avec f la **BRDF** (Bidirectional Reflectance Distribution Function) fonction telle que :

- f &ge; 0
- f(**w<sub>i</sub>** , **w<sub>o</sub>**) = f(**w<sub>o</sub>**,**w<sub>i</sub>**) (condition de réciprocité)
- &int; f(**w<sub>i</sub>** , **w<sub>o</sub>**) . cos(**w<sub>i</sub>**) d<sub>w<sub>i</sub></sub> &le; 1 pour tout **w<sub>o</sub>** (conservation de l'énergie).

Le calcul de l'intensité lumineuse sur un pixel de l'écran constitue une équation de Fredholm du 2<sup>ème</sup> type. En effet, le calcul de l'intensité d'un rayon nécessite le calcul de l'intensité d'un autre rayon.  
Il est ainsi possible de calculer l'émission du rayon arrivant sur l'écran en se limitant à un nombre de rebonds, par exemple **5 rebonds**, et en calculant récursivement l'intensité du rayon. Cela reprend ce qui a été fait pour les mirroirs et les surfaces transparentes.

La problématique est ici le fait qu'un nombre infini de rayon sont la cause de l'éclairage indirect, or pour des raisons évidentes de performance, il n'est pas possible de tous les prendre en compte. Ainsi, on se limite à un rayon par surface.

Le rayon **w<sub>i</sub>** est dirigé par les vecteurs **N**, **T1** et **T2** tel que :

**w<sub>i</sub>** = z.**N** + x.**T<sub>1</sub>** + y.**T<sub>2</sub>**

avec :

- x = cos(2.&pi;.r<sub>1</sub>) &radic;(1-r<sub>2</sub>)
- y = sin(2.&pi;.r<sub>1</sub>) &radic;(1-r<sub>1</sub>)
- z = &radic;(r<sub>2</sub>)

avec **z** dirigé par **N** puis

**T1** est calculé selon la valeur minimale de **N** afin de s'assurer que l'on ait pas **N** = (0,0,1) et ainsi **T** = **0** :

- **T1** = (-N<sub>y</sub>,N<sub>x</sub>,0) si N<sub>z</sub> est minimale
- **T1** = (N<sub>z</sub>,0,-N<sub>x</sub>) si N<sub>y</sub> est minimale
- **T1** = (0,-N<sub>z</sub>,N<sub>y</sub>) si N<sub>x</sub> est minimale

et **T2** = **N**&#10799;**T1**

Le calcul d'un seul rayon indirect par surface est amélioré en générant un certain nombre de rayons par pixel de l'écran, par exemple 100 rayons, qui se propagent différements (de manière aléatoire) et dont on moyenne les intensités lumineuses afin d'espérer obtenir l'intensité moyenne du pixel.

#### Generation de nombre aléatoire <a name="randomnumber"></a>

La génération de nombre aléatoire est réalisée en utilisant la **formule de Box Muller**.

On génère deux nombres aléatoires u<sub>1</sub> et u<sub>2</sub> suivant une loi uniforme sur [0,1]
puis on calcule deux nombres aléatoires :

x<sub>1</sub> = &sigma; . cos(2 &pi; u<sub>1</sub>) . &radic;(-2 log(u<sub>2</sub>))

x<sub>2</sub> = &sigma; . sin(2 &pi; u<sub>1</sub>) . &radic;(-2 log(u<sub>2</sub>))

x<sub>1</sub> et x<sub>2</sub> suivent alors une loi Gaussienne d'écart-type &sigma;.

#### Résultat de l'éclairage indirect <a name="resulteclairageindirect"></a>

Les résultats ci-dessous présentent le résultats de l'implémentation de l'éclairage indirect.

Avec un seul rayon indirect généré aléatoirement par surface, on obtient le résultat ci-dessous en environ 1 seconde. L'image semble bruitée, le résultat n'est pas suffisament performant.

![raytracer_eclairage_indirect_1_rayon](Figures/raytracer_eclairage_indirect_1_rayon.png)

Avec 10 rayons indirects générés aléatoirement par surface, on obtient le résultat ci-dessous en environ 10 secondes. L'image semble toujours bruitée mais le résultat est plus satisfaisant que le précédent.

![raytracer_eclairage_indirect_10_rayons](Figures/raytracer_eclairage_indirect_10_rayons.png)

Avec 100 rayons indirects générés aléatoirement par surface, on obtient le résultat ci-dessous en environ 100 secondes. On aperçoit encore légèrement le bruit même si le résultat est très satisfaisant.

![raytracer_eclairage_indirect_100_rayons](Figures/raytracer_eclairage_indirect_100_rayons.png)

Lorsque l'on compare ce résultat avec l'image sans éclairage indirect, on s'aperçoit entre autre que le sol, qui apparaissait blanc (sa couleur de définition), possède maintenant une teinte influencée par la couleur des murs qui l'entourent (bleu et rouge).

L'image ci-dessous présente la combinaison de l'éclairage indirect et de la transmission de Fresnel au niveau de la sphère transparente, en combinant les méthodes aléatoires pour optimiser les calcules, on obtient le résultat en 80 secondes, 20 secondes de moins que précédemment.

![raytracer_eclairage_indirect_100_rayons_Fresnel_aleatoires](Figures/raytracer_eclairage_indirect_100_rayons_Fresnel_aleatoires.png)

### Parallélisation des calculs <a name="parallelisation"></a>

Chaque pixel de l'image étant calculé de manière indépendante des autres pixels, on peut paralléliser la boucle de l'algorithme afin que les calculs soient exécutés en parallèle.

En appliquant cette méthode, on parvient à réduire le temps d'exécution de notre algorithme à 10 secondes contre 80 secondes précédemment.

### Crénelage <a name="crenelage"></a>

Lorsque l'on observe une des images générées précédemment, on s'aperçoit que les bords des surfaces sont crénelés (en forme d'escalier).

La raison de ce phénomène est que les rayons intersectent l'écran au milieu des pixels et deux rayons issues de deux pixels voisins peuvent, avec la distance entre l'écran et la surface, ne pas tout les deux appartenir à la surface ou inversement.

Ainsi, pour supprimer ce crénelage, il convient de ne pas faire passer tout les rayons par le centre du pixel mais de les faire passer par un point du pixel de sorte à ce que la distribution des intersections des rayons sur le pixel suive une loi Gaussienne centrée sur le pixel.

Les 100 rayons générés pour l'éclairage indirect sont ainsi utilisés avec des coordonnées légèrement différentes afin de d'augmenter la diversité des rayons.

On obtient le résultat ci-dessous, le crénelage n'est plus visible.

![raytracer_crenelage](Figures/raytracer_crenelage.png)

## Feedback sur le MOS <a name="feedback"></a>

- Points positifs :

  - cours très intéressant
  - le mélange de théorie et de mise en pratique simultané rend le travail très stimulant et l'évolution de l'image d'un modèle très simple à un modèle plus complexe rend le court d'autant plus motivant

- Point négatif :
  - la connaissance du C++ et des outils de développement est plus ou moins prise pour acquis, ce qui n'est pas le cas pour tout le monde.

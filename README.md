# MOS 2.2 - Informatique Graphique

Enseignant : Nicolas Bonneel

Etudiant : Julien Verdun

Date : 06/01/2021

## Ray Tracing

Le ray tracing est une technique de calcul d'optique par ordinateur, utilisée pour le rendu en synthèse d'image ou pour des études de systèmes optiques

On décrit dans cette partie l'implémentation d'un algorithme de **Ray Tracing**. Les différentes étapes permettant de complexifier notre algorithme sont expliquées.

Le fichier `raytracer.cpp` contient les classes **C++** et le **main** nécessaires pour créer les images présentées plus bas.

### Ray Tracing primitif

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

### Ajout de l'intensité lumineuse

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

### Création d'une scène

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

# Awale

to compil -> make
to launch awale -> go in this repo and ./awale
to launch server -> go in repo "Server" and ./server
to launch client -> go in repo "Client" and ./client 127.0.0.1 

warning : listen socket is like a FIFO for the connection and pseudo is unique

gérer les mutex à faire

à faire q4 (ça commence, il faut faire les coups, etc, mtn), 5, 6, 8, 9 et 10 si on a le temps


je voudrais renvoyer un message pour un coup invalide (qu'il puisse rejouer avec explication)
je voudrais que si un joueur se déconnecte, ça stoppe le jeu
maj des bonnes variables aussi à faire
score

            // Envoyer le score joueur 1 et joueur 2
            /*offset += snprintf(plateau_updated + offset, BUF_SIZE - offset, ":%d:%d", 
                jeu->score_joueur1, jeu->score_joueur2);*/
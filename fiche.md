## Fiche mentale — `mini_serv`

### 1. Vérifier les arguments

Le programme attend **un seul argument : le port**.

- Mauvais nombre d'arguments (!=2) → afficher le message imposé et quitter.
- Sinon → récupérer le port.

---

### 2. Créer le serveur

Construire la socket d'écoute en trois étapes :

**socket → bind → listen**

Mentalement :

> Je crée une socket TCP → je l'attache à `127.0.0.1:port` → je la mets en écoute.

Toute erreur système à cette phase → `Fatal error`.

À ce stade :

> `sockfd` représente la socket qui reçoit les **nouvelles demandes de connexion**.

---

### 3. Initialiser la surveillance

Créer deux ensembles :

- **master_set** = tous les fd que le serveur doit surveiller durablement ;
- **read_set** = copie temporaire donnée à `select`.

Au départ :

> `master_set` contient uniquement `sockfd`.

Conserver également :

- `max_fd` = plus grand fd surveillé ;
- `id` = prochain identifiant client.

---

### 4. Entrer dans la boucle principale

Le serveur tourne indéfiniment.

À chaque tour :

> copier `master_set` → `read_set`

Pourquoi ?

Parce que `select()` **modifie** l'ensemble qu'on lui donne.

Puis :

> appeler `select()` jusqu'à `max_fd` inclus.

Après son retour :

> `read_set` contient les fd qui sont prêts.

---

### 5. Parcourir les fd prêts

Parcourir les fd de `0` à `max_fd`.

Pour chaque fd :

> Est-il présent dans `read_set` ?

Non → rien à faire.

Oui → deux possibilités :

```text
sockfd	              client_fd
    │                     │
    ▼                     ▼
 nouvelle              données /
 connexion            déconnexion
    │                     │
  accept                 recv
```

C'est **la grande bifurcation du programme**.

---

## 6. Si `sockfd` est prêt → nouveau client

Faire `accept()`.

On obtient un nouveau `client_fd`.

Pour ce client :

- lui attribuer le prochain ID ;
- initialiser son buffer à vide/NULL ;
- ajouter son fd à `master_set` ;
- mettre à jour `max_fd` si nécessaire ;
- incrémenter le prochain ID.

Puis prévenir **tous les autres clients** :

> `server: client X just arrived\n`

Le nouveau client ne reçoit pas sa propre annonce.

---

## 7. Si un client est prêt → `recv`

Lire un morceau du flux dans un buffer temporaire.

Il y a essentiellement deux cas importants :

### `recv > 0`

Le client a envoyé des données.

→ traitement des données.

### `recv == 0`

Le client s'est déconnecté.

→ traitement de la déconnexion.

---

# 8. Données reçues : accumuler

TCP fournit un **flux**, pas des messages.

Un `recv()` peut donc donner :

```text
"hel"
```

puis le suivant :

```text
"lo\n"
```

On ne doit pas broadcaster `"hel"`.

Il faut conserver un **buffer persistant par client**.

À chaque `recv` :

> ancien buffer + nouveau morceau → nouveau buffer

C'est le rôle de `str_join`.

Exemple :

```text
buffer = "hel"
recv   = "lo\nwor"

→ buffer = "hello\nwor"
```

---

# 9. Extraire les lignes complètes

Une ligne n'est prête que lorsqu'on rencontre `\n`.

Utiliser `extract_message` en boucle.

Son rôle mental :

```text
buffer = "hello\nworld\nabc"

       ↓ extract_message

message = "hello\n"
buffer  = "world\nabc"

       ↓ extract_message

message = "world\n"
buffer  = "abc"

       ↓ extract_message

aucune ligne complète
```

Le reste incomplet demeure dans le buffer pour le prochain `recv`.

---

# 10. Broadcaster chaque ligne

Pour chaque ligne extraite :

1. construire le préfixe `client X: `;
2. envoyer le préfixe aux autres clients ;
3. envoyer la ligne extraite aux autres clients ;
4. libérer la ligne extraite.

Ne jamais renvoyer le message à son émetteur.

Donc :

```text
client 2 envoie "hello\n"

            ↓

autres clients reçoivent :

client 2: hello\n
```

---

# 11. Déconnexion

Quand :

> `recv == 0`

le client n'existe plus.

Avant de le supprimer, récupérer son ID et annoncer aux autres :

> `server: client X just left\n`

Puis :

- libérer son buffer ;
- retirer son fd de `master_set`;
- fermer son fd ;
- éventuellement recalculer `max_fd` s'il était le plus grand.

Le serveur, lui, **continue de fonctionner**.

---

# 12. Le modèle mental à retenir

Si tu ne devais retenir qu'une seule représentation, retiens celle-ci :

```text
INITIALISATION
│
├─ vérifier arguments
├─ socket
├─ bind
├─ listen
├─ initialiser master_set
└─ ajouter server_fd
        │
        ▼
┌──── BOUCLE INFINIE ────┐
│                        │
│ master → read_set      │
│        │               │
│      select            │
│        │               │
│ parcourir fd prêts     │
│        │               │
│   ┌────┴────┐          │
│   │         │          │
│ serveur   client       │
│   │         │          │
│ accept     recv        │
│   │         │          │
│ annoncer   ┌┴────────┐ │
│ arrivée    │         │ │
│          > 0        = 0│
│            │         │ │
│          join      annoncer
│            │       départ
│         extract      │ │
│            │       free
│         lignes      CLR
│            │       close
│        broadcast      │
│            │          │
└────────────┴──────────┘
```

## La séquence à savoir reconstruire

Quand tu t'entraînes sur fichier vierge, essaie simplement de réciter :

**Arguments → socket → bind → listen → fd_sets → boucle → copie → select → parcours → accept ou recv → join → extract → broadcast → déconnexion/nettoyage.**

Mais ne mémorise pas ça comme une formule magique. Chaque étape répond à une nécessité :

**`select`** attend l'activité → **`accept`** crée les connexions → **`recv`** récupère le flux → **`str_join`** reconstitue les morceaux → **`extract_message`** transforme le flux en lignes → **`broadcast`** distribue les lignes → **`clean_client`** supprime les connexions terminées.

Ton prochain entraînement devrait être de **fermer le code actuel et essayer de reconstruire `mini_serv` uniquement avec cette fiche**, quitte à compiler plusieurs fois. C'est exactement le passage qu'on cherche : passer de « je comprends le code quand je le vois » à « je sais le reconstruire à partir du problème ».
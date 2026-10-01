s_client{
	int		id;
	char	*buffer;
}			t_client;

# int	extract_message()
 récupérer cette fonction directement depuis le main.c

# char	*str_join()
 récupérer cette fonction directement depuis le main.c 

# void	broadcast(...)
 boucle pour envoyer le message à tous les autres clients (pas au socket) 

# void	clean_client(...)
 fonction pour supprimer le client de l'ensemble "master_set", fermer son fd et libérer son buffer si nécessaire 

# void	clean_all_clients(...)
 boucle sur tous les clients pour éxecuter clean_client(), puis supprime la socket serveur de l'ensemble "master_set" et ferme son fd 

# void	fatal_error()
 écrit "Fatal error" sur stderr et exit 1 

# int	main(...)
{
 ## vérifier les arguments 

 ## socket, bind, listen
   avec fatal_error à chaque fois en cas d'erreur 

 ## initialiser l'ensemble "master_set" 

 ## boucle while
{
read_set = master_set;
select(...) 
	fatal_error() si < 0; 

  ### boucle for pour parcourir tous les fds jusqu'à max_fd
   #### si le fd n'est pas prêt on passe au suivant.
   #### si le fd est la socket serveur, accept puis annoncer aux autres clients.
   #### sinon (client) recv
   ##### si (bytes == 0) -> déconnexion et prévenir tous les autres clients (max_fd à vérifier)
   ##### si (bytes > 0) -> on stocke dans le buffer (en passant par un nouveau buffer pour éviter de perdre le buffer actuel en cas d'erreur). Puis boucle tant que    extract message == 1 et on envoie chaque ligne aux autres clients avant de la free
			

 On pense à fermer la socket serveur à la fin. 
}

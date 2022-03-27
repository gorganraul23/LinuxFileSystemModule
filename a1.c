#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

char *get_substring(const char* src, char *dest, int start, int nr){
	int size = strlen(src);
	if(start + nr > size)
		return NULL;
	strncpy(dest, src + start, nr);
	strcat(dest, "\0");
	return dest;
}

unsigned int convert_string_octal(char *string){
	unsigned int res = 0;
	int c, b = 1;
	long long numar;
	sscanf(string, "%lld", &numar);
	while(numar != 0){
		c = numar % 10;
		numar /= 10;
		res += c * b;
		b *= 2;
	}
	return res;
}

int get_permissions(const char *permissions){
	char temp[10] = "";
	for(int i = 0; i < strlen(permissions); i++){
		if(permissions[i] == 'r'){
			if((i+1)%3 == 1)
				strcat(temp, "1");
			else{
				perror("Invalide permission");
				return -1;
			}
		}
		else if(permissions[i] == 'w'){
			if((i+1)%3 == 2)
				strcat(temp, "1");
			else{
				perror("Invalide permission");
				return -1;
			}
		}
		else if(permissions[i] == 'x'){
			if((i+1)%3 == 0)
				strcat(temp, "1");
			else{
				perror("Invalide permission");
				return -1;
			}
		}
		else if(permissions[i] == '-'){
			strcat(temp, "0");
		}
		else{
			perror("Invalide permission");
			return -1;
		}
	
	}
	strcat(temp, "\0");
	return convert_string_octal(temp);
}
								
		
bool parse(const char* path, bool extract, bool findall){	    //////////////////////////////////////////////// parse function
	int fd = -1, size;
	short int version;
	char buff[64], type, no_of_sect;
	fd = open(path, O_RDONLY);
	if(fd == -1){
        return false;
    }
    
    memset(buff, 0, 64);
    if(read(fd, &buff, 4) != 4){
    	printf("ERROR\nunsuccessful read\n");	//citesc magic
    	close(fd);
    	return false;
    }
    if(strcmp(buff, "AXW2") != 0){
    	if(!findall)
    		printf("ERROR\nwrong magic\n");		//verific ca mafic sa fie ok
    	close(fd);
    	return false;
    }
  
    lseek(fd, 2, SEEK_CUR);			//sar peste header_size
    memset(buff, 0, 64);
    if(read(fd, &version, 2) != 2){
    	printf("ERROR\nunsuccessful read\n");	//citesc version
    	close(fd);
    	return false;
    }
    
    if(version < 80 || version > 116){
    	if(!findall)
    		printf("ERROR\nwrong version\n");	//verific daca version e ok
    	close(fd);
    	return false;
    }
    
    if(read(fd, &no_of_sect, 1) != 1){
    	printf("ERROR\nunsuccessful read\n");	//citesc no_of_sections
    	close(fd);
    	return false;
    }
    if(no_of_sect < 2 || no_of_sect > 11){
    	if(!findall)
    		printf("ERROR\nwrong sect_nr\n");	//verific daca no_of_sections e ok
    	close(fd);
    	return false;
    }
 
    lseek(fd, 18, SEEK_CUR);
    for(int i = 0; i < no_of_sect; i++){			//verific daca type e ok
    	if(read(fd, &type, 1) != 1){
    		printf("ERROR\nunsuccessful read\n");
    		close(fd);
   			return false;
   		}
   		if(type != 18 && type != 29 && type != 64 && type != 86){
   			if(!findall)
   				printf("ERROR\nwrong sect_types\n");	
    		close(fd);
   			return false;
   		}
   		lseek(fd, 26, SEEK_CUR);	//4+4+18
    }
    lseek(fd, 9, SEEK_SET);		//4+2+2+1 revin 
    
    if(!extract)
    	printf("SUCCESS\nversion=%d\nnr_sections=%d\n", version, no_of_sect);
    for(int i = 1; i <= no_of_sect; i++){				//pt fiecare sectiune, extrag si printez datele
    	if(!extract)
    		printf("section%d: ", i);
    	if(read(fd, &buff, 18) != 18){
    		printf("ERROR\nunsuccessful read\n");	//citesc numele
    		close(fd);
    		return false;
   		}
   		if(!extract)
   			printf("%s ", buff);		//scriu numele
   		memset(buff, 0, 64);
   		
   		if(read(fd, &type, 1) != 1){
    		printf("ERROR\nunsuccessful read\n");	//citesc type
    		close(fd);
    		return false;
   		}
   		if(!extract)
   			printf("%d ", type);		//scriu type
   		
   		lseek(fd, 4, SEEK_CUR);
   		if(read(fd, &size, 4) != 4){
    		printf("ERROR\nunsuccessful read\n");	//citesc size
    		close(fd);
    		return false;
   		}
   		if(extract && findall){
   			if(size > 1130){
   				return false;	
   			}	
   		}
   		if(!extract)
   			printf("%d\n", size);		//scriu size
    } 	
  	close(fd);
  	return true;
}

								
bool is_valide(const char *path){		//verifica daca fisierul e valid si are dim ceruta
	if(parse(path, true, true)){
		return true;
	}
	return false;
}								

								////////////////////////////////////////////// list function		
int success = 0;
void list(const char *path, bool rec, bool size_g, bool perm, bool findall, const char *size_greater, const char *permissions)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char fullPath[512];
    struct stat statbuf;

    dir = opendir(path);
    if(dir == NULL) {
        fprintf(stderr, "ERROR\ninvalid directory path\n");
        return;
    }
    off_t size = 0;
    if(size_g){
    	sscanf(size_greater, "%ld", &size);
    }
    if(success == 0 && !findall){
  		printf("SUCCESS\n");
  		success++;
  	}
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
            if(lstat(fullPath, &statbuf) == 0) {
            
                if(S_ISDIR(statbuf.st_mode)){ 					// daca e folder
                	if(rec){
                		if(perm){
                			if((statbuf.st_mode & 0777) == get_permissions(permissions)){
                    			list(fullPath, rec, size_g, perm, findall, size_greater, permissions);
                    			if(!size_g)
                    				printf("%s\n", fullPath);
                    		}
                    	}
                    	else{
                  			list(fullPath, rec, size_g, perm, findall, size_greater, permissions);
                  			if(!size_g && !findall)
                  				printf("%s\n", fullPath);
                  		}
                    }
                    else{
                    	if(!size_g)
                    	if((perm && ((statbuf.st_mode & 0777) == get_permissions(permissions))) || perm == false)
                    		printf("%s\n", fullPath);
                    }
                }
                else{											//daca e fisier
                	if(size_g){
                		if(statbuf.st_size > size){
                			if(perm){	
                				if((statbuf.st_mode & 0777) == get_permissions(permissions)){
                					printf("%s\n", fullPath);
                				}
                			}
                			else
                				printf("%s\n", fullPath);
                		}
                	}
                	else if(perm){
        				//printf("a");
                		if((statbuf.st_mode & 0777) == get_permissions(permissions)){
                			printf("%s\n", fullPath);
                		}
                	}
                	else{
                		if(findall){ 
                			if(is_valide(fullPath)){	//daca e SF si sectiunile au dim buna
                				if(success == 0){
                					printf("SUCCESS\n");
                					success++;
                				}
                				printf("%s\n", fullPath);
                			}
                		}
                		else	
                			printf("%s\n", fullPath);
                	}	
                }
            }
        }
    }	
    closedir(dir);
}

void extract(const char *path, int section, int line){
        int fd = -1;
	if(parse(path, true, false)){						//verific daca fisierul e SF valid
		int offset = 0, size = 0, nr_linie = 1;
		short int header_size = 0;
		char no_of_sect, c;
		
		fd = open(path, O_RDONLY);
		if(fd == -1){
        	printf("ERROR\ninvalid file path\n");
        	return;
        }
        
        lseek(fd, 4, SEEK_SET);				//trec peste magic
        if(read(fd, &header_size, 2) != 2){
        	printf("ERROR\nunsuccessful read\n");	//citesc header_size
    		close(fd);
    		return;
        }
        lseek(fd, 2, SEEK_CUR);			//trec peste version
        if(read(fd, &no_of_sect, 1) != 1){
        	printf("ERROR\nunsuccessful read\n");	//citesc nr de sectiuni
    		close(fd);
    		return;
        }
        if(section < 1 || section > no_of_sect){		//verific daca se da o sectiune valida
        	printf("ERROR\ninvalid section\n");	
    		return;
        }
        
        
        lseek(fd, (section-1)*27, SEEK_CUR);		//ma deplasez la header_section dorit
        lseek(fd, 19, SEEK_CUR);				//trec peste sect_name si type
        
        if(read(fd, &offset, 4) != 4){
        	printf("ERROR\nunsuccessful read\n");	//citesc offset
    		close(fd);
    		return;
        }
        if(read(fd, &size, 4) != 4){
        	printf("ERROR\nunsuccessful read\n");	//citesc sect_size
    		close(fd);
    		return;
        }

        lseek(fd, offset, SEEK_SET);	//ma deplasez la inceputul sectiunii
        
        char buff[size+1];
        int pos = 0, read_size = 0;
        bool efectuat = false;
        
        while(true){
               if(pos < size-1){
        	read_size = read(fd, &c, 1);}
        	else{break;}
        	if(read_size < 0)
        		break;
        	else if(read_size == 0){
        		if(nr_linie == line)
             		efectuat = true;
           		strcat(buff, "\0");
           		break;
        	}
        	else{
        		if(nr_linie == line){
        			if(c != 10 && c != 0){           //////////////
        				buff[pos] = c;
        				pos++;
        			}
        			else{
           				efectuat = true;
           				strcat(buff, "\0");
           				break;
           			}
        		}
        		else{
        			if(c == 10 ){
        				nr_linie++;}
        				if(c == 0){
        				break;}
        		}	
        	}	
        }
        
        if(efectuat){
        	printf("SUCCESS\n");
        	for(int i = pos-1; i >=0 ; i--){
        		printf("%c", buff[i]); 
        	}
        	printf("\n");
        	close(fd);
        }
        else{
        	printf("ERROR\ninvalid line\n");
        	close(fd);	
    		return;
        }
        	     
    }
    else{
    	printf("ERROR\ninvalid file\n");	
    	return;
    }
}


int main(int argc, char **argv){
    if(argc >= 2){
    
        if(strcmp(argv[1], "variant") == 0){					//////////////////////////////////////variant
            printf("38268\n");
        }
        														//////////////////////////////////////list
        if(strcmp(argv[1], "list") == 0){
        	bool rec = false, size_g = false, perm = false;	
        	char dest[501] = "", path[501] = "", permissions[10] = "", size_greater[15] = "";		
        	if(argc == 3){
        		if(strcmp(get_substring(argv[2], dest, 0, 5), "path=") == 0){
        			memset(dest, 0, 501);
        			strcpy(path, get_substring(argv[2], dest, 5, strlen(argv[2])-5));
        		}	
        		else{
        			fprintf(stderr, "ERROR\ninvalid options\n");
        			return -1;
        		}
        	}
        	
        	else{
        		for(int i = 2; i <= argc-2; i++){
        			if(strcmp(argv[i], "recursive") == 0){
        				rec = true;
        			}
        			else if(strcmp(get_substring(argv[i], dest, 0, 12), "permissions=") == 0){
        				perm = true;
        				memset(dest, 0, 501);
        				strcpy(permissions, get_substring(argv[i], dest, 12, strlen(argv[i])-12));
        			}
        			else if(strcmp(get_substring(argv[i], dest, 0, 13), "size_greater=") == 0){
        				size_g = true;
        				memset(dest, 0, 501);
        				strcpy(size_greater, get_substring(argv[i], dest, 13, strlen(argv[i])-13));
        			}
        			else{
        				fprintf(stderr, "ERROR\ninvalid options\n");
        				return -1;
        			}
        		}
        		
        		memset(dest, 0, 501);
        		if(strcmp(get_substring(argv[argc-1], dest, 0, 5), "path=") == 0){
        			memset(dest, 0, 501);
        			strcpy(path, get_substring(argv[argc-1], dest, 5, strlen(argv[argc-1])-5));
        		}
        		else{
        			fprintf(stderr, "ERROR\ninvalid arguments\n");
        			return -1;
        		}	
        	}
        	list(path, rec, size_g, perm, false, size_greater, permissions);
        }														////////////////// end list
        
        															
        if(strcmp(argv[1], "parse") == 0){							/////////////////////////////////parse
        	if(argc == 3){
        		char dest[501] = "", path[501] = "";
        		if(strcmp(get_substring(argv[argc-1], dest, 0, 5), "path=") == 0){
        			memset(dest, 0, 501);
        			strcpy(path, get_substring(argv[argc-1], dest, 5, strlen(argv[argc-1])-5));
        		}
        		else{
        			fprintf(stderr, "ERROR\ninvalid arguments\n");
        			return -1;
        		}	
        		parse(path, false, false);
        	}
        	else{
        		fprintf(stderr, "ERROR\nwrong number of arguments\n");
        		return -1;
        	}
        }
        
        
        if(strcmp(argv[1], "extract") == 0){					/////////////////////////////////////////extract
        	if(argc == 5){
        		char dest[501] = "", path[501] = "", section[501] = "", line[5]= "";
        		int sec = 0, l = 0;
       
        		if(strcmp(get_substring(argv[2], dest, 0, 5), "path=") == 0){
        			memset(dest, 0, 501);
        			strcpy(path, get_substring(argv[2], dest, 5, strlen(argv[2])-5));				
        		}
        		else{
        			fprintf(stderr, "ERROR\ninvalid options\n");
       				return -1;
       			}
       			memset(dest, 0, 501);
       			if(strcmp(get_substring(argv[3], dest, 0, 8), "section=") == 0){
       				memset(dest, 0, 501);
       				strcpy(section, get_substring(argv[3], dest, 8, strlen(argv[3])-8));			
        		}
        		else{
       				fprintf(stderr, "ERROR\ninvalid options\n");
       				return -1;
       			}
        		memset(dest, 0, 501);
        		if(strcmp(get_substring(argv[4], dest, 0, 5), "line=") == 0){
        			memset(dest, 0, 501);
       				strcpy(line, get_substring(argv[4], dest, 5, strlen(argv[4])-5)); 				
        		}
        		else{
        			fprintf(stderr, "ERROR\ninvalid options\n");
       				return -1;
       			}
       			
       			sscanf(section, "%d", &sec);
        		sscanf(line, "%d", &l);
       			
       			extract(path, sec, l);
       		}
       		else{
       			fprintf(stderr, "ERROR\nwrong number of arguments\n");
       			return -1;
    		}													////////////////////////end extract
       	}
       	
       	if(strcmp(argv[1], "findall") == 0){				////////////////////////////////////////findall
       		char dest[501] = "", path[501] = "";
       		if(argc == 3){			
       			if(strcmp(get_substring(argv[2], dest, 0, 5), "path=") == 0){
        			memset(dest, 0, 501);
        			strcpy(path, get_substring(argv[2], dest, 5, strlen(argv[2])-5));				
        		}
        		else{
        			fprintf(stderr, "ERROR\ninvalid options\n");
       				return -1;
       			}
       			
       			list(path, true, false, false, true, NULL, NULL);
       			if(success == 0)
    				printf("SUCCESS\n");
       		}
       		else{
       			fprintf(stderr, "ERROR\nwrong number of arguments\n");
       			return -1;
    		}	
       	}
       	
       	
    }    	
     																
    return 0;
}

/*
 * From: nob...@vox.xs4all.nl (An0nYm0Us UsEr)
 * Newsgroups: sci.crypt
 * Subject: RC4 ?
 * Date: 13 Sep 1994 21:30:36 GMT
 * Organization: Global Anonymous Remail Services Ltd.
 * Lines: 83
 * Message-ID: <3555ls$fsv@news.xs4all.nl>
 * NNTP-Posting-Host: xs1.xs4all.nl
 * X-Comment: This message did not originate from the above address.
 * X-Comment: It was automatically remailed by an anonymous mailservice.
 * X-Comment: Info: us...@xs4all.nl, Subject: remailer-help 
 * X-Comment: Please report inappropriate use to <ad...@vox.xs4all.nl>
*/

namespace jerboa_random {

typedef struct rc4_key
{      
     unsigned char state[256];      
     unsigned char x;        
     unsigned char y;
} rc4_key;

void swap_byte(unsigned char *a, unsigned char *b)
{
     unsigned char swapByte;
     
     swapByte = *a;
     *a = *b;      
     *b = swapByte;
}

void prepare_key(unsigned char *key_data_ptr, int key_data_len, rc4_key *key)
{
     unsigned char swapByte;
     unsigned char index1;
     unsigned char index2;
     unsigned char* state;
     short counter;    
     
     state = &key->state[0];        
     for(counter = 0; counter < 256; counter++)              
     state[counter] = counter;              
     key->x = 0;    
     key->y = 0;    
     index1 = 0;    
     index2 = 0;            
     for(counter = 0; counter < 256; counter++)      
     {              
          index2 = (key_data_ptr[index1] + state[counter] +
index2) % 256;                
          swap_byte(&state[counter], &state[index2]);            

          index1 = (index1 + 1) % key_data_len;  
     }      
 }
 
void rc4(unsigned char *buffer_ptr, int buffer_len, rc4_key *key)
{
     unsigned char x;
     unsigned char y;
     unsigned char* state;
     unsigned char xorIndex;
     short counter;              
     
     x = key->x;    
     y = key->y;    
     
     state = &key->state[0];        
     for(counter = 0; counter < buffer_len; counter ++)      
     {              
          x = (x + 1) % 256;                      
          y = (state[x] + y) % 256;              
          swap_byte(&state[x], &state[y]);                        
               
          xorIndex = state[x] + (state[y]) % 256;                
               
          buffer_ptr[counter] ^= state[xorIndex];        
      }              
      key->x = x;    
      key->y = y;
}
 
rc4_key Engine;

}

void RandomSetup() {
  jerboa_random::prepare_key("RandomSetup", 12, &jerboa_random::Engine);
}

byte RandomByte() {
  unsigned char buf[1] = {0};
  jerboa_random::rc4(buf, 1, &jerboa_random::Engine);
  return buf[0];
}

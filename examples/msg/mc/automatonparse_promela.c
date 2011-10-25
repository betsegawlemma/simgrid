#include "automatonparse_promela.h"

char* state_id_src;

void init(){
  automaton = xbt_automaton_new_automaton();
}

void new_state(char* id, int src){

  char* id_state = strdup(id);
  char* first_part = strtok(id,"_");
  int type = 0 ; // -1=état initial, 0=état intermédiaire, 1=état final

  if(strcmp(first_part,"accept")==0){
    type = 1;
  }else{
    char* second_part = strtok(NULL,"_");
    if(strcmp(second_part,"init")==0){
      type = -1;
    }
  }

  xbt_state_t state = NULL;
  state = xbt_automaton_state_exists(automaton, id_state);
  if(state == NULL){
    state = xbt_automaton_new_state(automaton, type, id_state);
  }

  if(type==-1)
    automaton->current_state = state;

  if(src)
    state_id_src = strdup(id_state);
    
}

void new_transition(char* id, xbt_exp_label_t label){

  char* id_state = strdup(id);
  xbt_state_t state_dst = NULL;
  new_state(id, 0);
  state_dst = xbt_automaton_state_exists(automaton, id_state);
  xbt_state_t state_src = xbt_automaton_state_exists(automaton, state_id_src); 
  
  xbt_transition_t trans = NULL;
  trans = xbt_automaton_new_transition(automaton, state_src, state_dst, label);

}

xbt_exp_label_t new_label(int type, ...){
  xbt_exp_label_t label = NULL;
  va_list ap;
  va_start(ap,type);
  switch(type){
  case 0 : {
    xbt_exp_label_t left = va_arg(ap, xbt_exp_label_t);
    xbt_exp_label_t right = va_arg(ap, xbt_exp_label_t);
    label = xbt_automaton_new_label(type, left, right);
    break;
  }
  case 1 : {
    xbt_exp_label_t left = va_arg(ap, xbt_exp_label_t);
    xbt_exp_label_t right = va_arg(ap, xbt_exp_label_t);
    label = xbt_automaton_new_label(type, left, right);
    break;
  }
  case 2 : {
    xbt_exp_label_t exp_not = va_arg(ap, xbt_exp_label_t);
    label = xbt_automaton_new_label(type, exp_not);
    break;
  }
  case 3 : {
    char* p = va_arg(ap, char*);
    label = xbt_automaton_new_label(type, p);
    break;
  }
  case 4 : {
    label = xbt_automaton_new_label(type);
    break;
  }
  }
  va_end(ap);
  return label;
}

xbt_automaton_t get_automaton(){
  return automaton;
}

void display_automaton(){
  xbt_automaton_display(automaton);
}



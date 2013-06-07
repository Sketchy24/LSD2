/***************************************************
****************************************************
LSD 6.1 - June 2011
written by Marco Valente
Universita' dell'Aquila

Copyright Marco Valente
Lsd is distributed according to the GNU Public License

Comments and bug reports to marco.valente@univaq.it
****************************************************
****************************************************/



/****************************************************
SETALL.CPP
It contains the routine called from the edit_dat file for setting all the
values of a variable with a function, instead of inserting manually.

The functions contained in this file are:

-void set_all(int *choice, object *r, char *lab, int lag)
it allows 5 options to set all values. It uses one value entered by the user
in this window and, for some option, the first value for this variable in the
model. That is, the value for this variable contained in the first object of this
type.
The options are the following:
1) set all values equal to the entered value
2) the first value is not changed and all the others are computed as the previous
plus the entered object.
3) as before, but instead of producing a ever increasing series, it re-initialize
any new group.
4) random numbers, drawn by a uniform value whose min is the first value
and max is the inserted value
5) random numbers, drawn by a normal whose mean is the first value and
standard deviation is the inserted value.



-object *create(Tcl_Interp *inter, object *root)
The main cycle for the Browser, from which it exits only to run a simulation
or to quit the program. The cycle is just once call to browsw followed by
a call to operate.

- int browse(Tcl_Interp *inter, object *r, int *choice);
build the browser window and waits for an action (on the form of
values for choice or choice_g different from 0)

- object *operate(Tcl_Interp *in, int *choice, object *r);
takes the value of choice and operate the relative command on the
object r. See the switch for the complete list of the available commands

- void clean_debug(object *n);
remove all the flags to debug from any variable in the model

- void clean_save(object *n);
remove all the flags to save from any variable in the model

- void clean_plot(object *n);
remove all the flags to plot from any variable in the model


Functions used here from other files are:

- void cmd(Tcl_Interp *inter, char *cc);
UTIL.CPP Standard routine to send the message string cc to the interp
Basically it makes a simple Tcl_Eval, but controls also that the interpreter
did not issue an error message.

- double norm(double mean, double dev)
UTIL.CPP returns a random number drawn from a normal with mean mean and standard deviation\
dev.

- double rnd(void);
UTIL.CPP return the uniform value. Now is only implemented using the internal
random generator, but it can be (and should...) linked with a serious random
generator.
****************************************************/


#include <tk.h>
#include "decl.h"

extern long idum;
extern Tcl_Interp *inter;
extern char msg[];
extern int t;

void cmd(Tcl_Interp *inter, char const *cc);
double ran1(long *idum);
#define RND ran1(&idum)
double rnd_integer(double min, double max);
double norm(double mean, double dev);
void plog(char const *msg);
description *search_description(char *lab);
void change_descr_lab(char const *lab_old, char const *lab, char const *type, char const *text, char const *init);
int compute_copyfrom(object *c, int *choice);
void set_window_size(void);
void add_description(char const *lab, char const *type, char const *text);

int init_random(int seed);

/****************************************************
SET_ALL

****************************************************/

void set_all(int *choice, object *original, char *lab, int lag)
{
char *l, ch[300];
int res, i, kappa, cases_from=1, cases_to=0, to_all, update_description, fill=0;
object *cur, *r;
double value, value1, value2, step, counter;
variable *cv;
FILE *f;
description *cd; 

if(original->up!=NULL)
 for(r=original->up; r->up!=NULL; r=r->up);//go for the root
else
 r=original; 
r=r->search(original->label);//select the first instance

Tcl_LinkVar(inter, "res", (char *) &res, TCL_LINK_INT);
Tcl_LinkVar(inter, "value1", (char *) &value1, TCL_LINK_DOUBLE);
Tcl_LinkVar(inter, "value2", (char *) &value2, TCL_LINK_DOUBLE);


res=1;
value1=0;
value2=0;

cmd(inter, "wm title . \"Lsd - Set All initialization\"");
cv=r->search_var(NULL, lab);



cmd(inter, "frame .f1");
cmd(inter, "frame .head");
cmd(inter, "label .head.lg -text \"Set initial values for every copy of \"");
if(cv->param==1)
  sprintf(ch, "label .head.l -text \"Parameter %s\" -fg red", lab);
else
  sprintf(ch, "label .head.l -text \"Var: %s (lag %d)\" -fg red", lab, t-cv->last_update+lag);
cmd(inter, ch);

sprintf(ch, "label .head.lo -text \"contained in Object %s\"", cv->up->label);
cmd(inter, ch);

cmd(inter, "pack .head.lg .head.l .head.lo");

cmd(inter, "set value 1");
cmd(inter, "frame .f1.rd -relief groove -bd 2");

cmd(inter, "label .f1.rd.l -text \"Available initialization functions\" -fg red");

cmd(inter, "radiobutton .f1.rd.r1 -text \"Equal to \" -variable res -value 1 -command { .f1.val.l1 conf -text \"Value\"; .f1.val.l2 conf -text \"(no use)\"}");
cmd(inter, "bind .f1.rd.r1 <Down> {focus -force .f1.rd.r9; .f1.rd.r9 invoke}");
cmd(inter, "bind .f1.rd.r1 <Return> { .f1.val.e1 selection range 0 end; focus .f1.val.e1}");

cmd(inter, "radiobutton .f1.rd.r9 -text \"Range \" -variable res -value 9 -command { .f1.val.l1 conf -text \"Minimum\"; .f1.val.l2 conf -text \"Maximum\"}");
cmd(inter, "bind .f1.rd.r9 <Down> {focus -force .f1.rd.f2.r2; .f1.rd.f2.r2 invoke}");
cmd(inter, "bind .f1.rd.r9 <Up> {focus -force .f1.rd.r1; .f1.rd.r1 invoke}");
cmd(inter, "bind .f1.rd.r9 <Return> { .f1.val.e1 selection range 0 end; focus .f1.val.e1}");


cmd(inter, "frame .f1.rd.f2");
cmd(inter, "radiobutton .f1.rd.f2.r2 -text \"Increasing \" -variable res -value 2 -command { .f1.val.l1 conf -text \"Start\"; .f1.val.l2 conf -text \"Step\"}");
cmd(inter, "bind .f1.rd.f2.r2 <Down> {focus -force .f1.rd.r4; .f1.rd.r4 invoke}");
cmd(inter, "bind .f1.rd.f2.r2 <Up> {focus -force .f1.rd.r9; .f1.rd.r9 invoke}");

cmd(inter, "bind .f1.rd.f2.r2 <Return> { .f1.val.e1 selection range 0 end; focus .f1.val.e1}");

cmd(inter, "set step_in 1");
cmd(inter, "pack .f1.rd.f2.r2 -side left");
cmd(inter, "radiobutton .f1.rd.r4 -text \"Increasing (Groups) \" -variable res -value 4 -command {.f1.val.l1 conf -text \"Start\"; .f1.val.l2 conf -text \"Step\"}");
cmd(inter, "bind .f1.rd.r4 <Up> {focus -force .f1.rd.f2.r2; .f1.rd.f2.r2 invoke}");
cmd(inter, "bind .f1.rd.r4 <Down> {focus -force .f1.rd.r3; .f1.rd.r3 invoke}");
cmd(inter, "bind .f1.rd.r4 <Return> {.f1.val.e1 selection range 0 end; focus .f1.val.e1}");

cmd(inter, "radiobutton .f1.rd.r3 -text \"Random (Uniform) \" -variable res -value 3 -command { .f1.val.l1 conf -text \"Min\"; .f1.val.l2 conf -text \"Max\"}");
cmd(inter, "bind .f1.rd.r3 <Up> {focus -force .f1.rd.r4; .f1.rd.r4 invoke}");
cmd(inter, "bind .f1.rd.r3 <Down> {focus -force .f1.rd.r8; .f1.rd.r8 invoke}");
cmd(inter, "bind .f1.rd.r3 <Return> { .f1.val.e1 selection range 0 end; focus .f1.val.e1}");

cmd(inter, "radiobutton .f1.rd.r8 -text \"Random Integer (Uniform) \" -variable res -value 8 -command { .f1.val.l1 conf -text \"Min\"; .f1.val.l2 conf -text \"Max\"}");
cmd(inter, "bind .f1.rd.r8 <Up> {focus -force .f1.rd.r3; .f1.rd.r3 invoke}");
cmd(inter, "bind .f1.rd.r8 <Down> {focus -force .f1.rd.r5; .f1.rd.r5 invoke}");
cmd(inter, "bind .f1.rd.r8 <Return> { .f1.val.e1 selection range 0 end; focus .f1.val.e1}");

cmd(inter, "radiobutton .f1.rd.r5 -text \"Random (Normal) \" -variable res -value 5 -command {.f1.val.l1 conf -text \"Mean\"; .f1.val.l2 conf -text \"Std Dev\"}");
cmd(inter, "bind .f1.rd.r5 <Up> {focus -force .f1.rd.r8; .f1.rd.r8 invoke}");
cmd(inter, "bind .f1.rd.r5 <Down> {focus -force .f1.rd.r7; .f1.rd.r7 invoke}");
cmd(inter, "bind .f1.rd.r5 <Return> { .f1.val.e1 selection range 0 end; focus .f1.val.e1}");


cmd(inter, "radiobutton .f1.rd.r7 -text \"File\" -variable res -value 7 -command { .f1.val.l1 conf -text \"(no use)\"; .f1.val.l2 conf -text \"(no use)\"}");
cmd(inter, "bind .f1.rd.r7 <Up> {focus -force .f1.rd.r5; .f1.rd.r5 invoke}");
cmd(inter, "bind .f1.rd.r7 <Return> {.f1.val.e1 selection range 0 end; focus .f1.val.e1}");


cmd(inter, "pack .f1.rd.l .f1.rd.r1 .f1.rd.r9 .f1.rd.f2 .f1.rd.r4 .f1.rd.r3 .f1.rd.r8 .f1.rd.r5 .f1.rd.r7 -anchor w");

cmd(inter, "bind .f1.rd <Return> { .f1.val.e1 selection range 0 end; focus .f1.val.e1}");


cmd(inter, "set rnd_seed 1");
cmd(inter, "set use_seed 0");
cmd(inter, "frame .f1.val -relief groove -bd 2");
cmd(inter, "frame .f2");
cmd(inter, "frame .f2.rnd -relief groove -bd 2");
cmd(inter, "label .f2.rnd.l -text \"Random generator settings\" -fg red");
cmd(inter, "checkbutton .f2.rnd.f -text \"Reset the generator\" -variable use_seed");
cmd(inter, "label .f2.rnd.l1 -text \"Seed used\"");
cmd(inter, "entry .f2.rnd.e1 -textvariable rnd_seed");
cmd(inter, "pack .f2.rnd.l .f2.rnd.f .f2.rnd.l1 .f2.rnd.e1 -expand yes -fill x");


cmd(inter, "label .f1.val.l -text \"Numerical data for initialization\" -fg red");
cmd(inter, "label .f1.val.l1 -text \"Value 1\"");
cmd(inter, "entry .f1.val.e1 -textvariable value1");
cmd(inter, "label .f1.val.l2 -text \"Value 2\"");
cmd(inter, "entry .f1.val.e2 -textvariable value2");
cmd(inter, "pack .f1.val.l .f1.val.l1 .f1.val.e1 .f1.val.l2 .f1.val.e2 -expand yes -fill x");

cmd(inter, "frame .f2.s -relief groove -bd 2");
cmd(inter, "set to_all 1");
cmd(inter, "label .f2.s.tit -text \"Extension of initialization\" -fg red");
cmd(inter, "pack .f2.s.tit");
cmd(inter, "radiobutton .f2.s.all -text \"Apply to every object \" -variable to_all -value 1");

cmd(inter, "bind .f2.s.all <1> {.f2.s.sel.to conf -state disabled; .f2.s.sel.from conf -state disabled}");
cmd(inter, "bind .f2.s.all <Down> {.f2.s.sel.to conf -state normal; .f2.s.sel.from conf -state normal;focus -force .f2.s.sel.sel; .f2.s.sel.sel invoke};");
cmd(inter, "bind .f2.s.all <Return> {focus -force .f2.b.ok}");

cmd(inter, "frame .f2.s.sel");
cmd(inter, "radiobutton .f2.s.sel.sel -text \"Apply to a group of objects\\n (use right button on cells)\" -variable to_all -value 2");
cmd(inter, "bind .f2.s.sel.sel <1> {.f2.s.sel.to conf -state normal; .f2.s.sel.from conf -state normal}");
cmd(inter, "pack .f2.s.sel.sel");
cmd(inter, "set cases_from 1; set cases_to 10000");
cmd(inter, "label .f2.s.sel.lfrom -text \" From: \"");
cmd(inter, "entry .f2.s.sel.from -width 5 -textvariable cases_from -state disabled");
cmd(inter, "label .f2.s.sel.lto -text \" to: \"");
cmd(inter, "entry .f2.s.sel.to -width 5 -textvariable cases_to -state disabled");
cmd(inter, "bind .f2.s.sel.sel <Return> {focus -force .f2.b.ok}");
cmd(inter, "bind .f2.s.sel.sel <Right> {focus -force .f2.s.sel.from}");
cmd(inter, "bind .f2.s.sel.from <Return> {focus -force .f2.s.sel.to}");
cmd(inter, "bind .f2.s.sel.from <Button-3> {set choice 9}");
cmd(inter, "bind .f2.s.sel.to <Button-3> {set choice 10}");
//cmd(inter, "bind .f2.s.sel.to <Left> {focus -force .f2.s.sel.from}");
cmd(inter, "bind .f2.s.sel.from <Left> {focus -force .f2.s.sel.sel}");
cmd(inter, "bind .f2.s.sel.sel <Up> {focus -force .f2.s.all}");
cmd(inter, "bind .f2.s.sel.to <Return> {focus -force .f2.b.ok}");

cmd(inter, "pack .f2.s.sel.lfrom .f2.s.sel.from .f2.s.sel.lto .f2.s.sel.to -side left");
cmd(inter, "pack .f2.s.all .f2.s.sel -anchor w");

cmd(inter, "set update_d 1");
cmd(inter, "checkbutton .f2.ud -text \"Update description\" -variable update_d");

cmd(inter, "frame .f2.fr -relief groove -bd 2");
cmd(inter, "label .f2.fr.tit -text \"Frequency\" -fg red");

cmd(inter, "label .f2.fr.l -text \"Apply every \"");
cmd(inter, "entry .f2.fr.e -width 6 -textvariable step_in");
cmd(inter, "label .f2.fr.l1 -text \" objects\"");
cmd(inter, "checkbutton .f2.fr.f -text \"Fill in\" -variable fill");
cmd(inter, "pack .f2.fr.tit");
cmd(inter, "pack  .f2.fr.l .f2.fr.e .f2.fr.l1 .f2.fr.f -side left  -expand yes -fill both");

cmd(inter, "bind . <KeyPress-Escape> {set choice 2}");
cmd(inter, "pack .f1.rd .f1.val");
cmd(inter, "pack .f2.rnd .f2.fr .f2.s .f2.ud -anchor w -expand yes -fill both");

cmd(inter, "pack .head");
cmd(inter, "pack .f1 .f2 -side left");
cmd(inter, "frame .f2.b");
cmd(inter, "button .f2.b.ok -text Ok -command {set choice 1}");
cmd(inter, "button .f2.b.help -text Help -command {LsdHelp mdatainit.html#setall}");
cmd(inter, "button .f2.b.can -text Cancel -command {set choice 2}");

cmd(inter, "pack .f2.b.ok .f2.b.help .f2.b.can -side left");
cmd(inter, "pack .f2.b -side bottom");
cmd(inter, "focus -force .f1.rd.r1; .f1.rd.r1 invoke");
cmd(inter, "bind .f1.val.e1 <KeyPress-Return> { .f1.val.e2 selection range 0 end; focus .f1.val.e2}");
cmd(inter, "bind .f1.val.e2 <KeyPress-Return> {focus -force .f2.s.all; .f2.s.all invoke}");
cmd(inter, "bind .f2.b.ok <KeyPress-Return> {.f2.b.ok invoke}");
cmd(inter, ".f1.rd.r1 invoke");

here_setall:
set_window_size();
while(*choice==0)
  Tcl_DoOneEvent(0);


if(*choice==9)
{//search instance from
 i=compute_copyfrom(original, choice);
 sprintf(msg, "set cases_from %d",i);
 cmd(inter, msg);
 *choice=0;
 goto here_setall;
}

if(*choice==10)
{
 //search instance to
 i=compute_copyfrom(original, choice);
 sprintf(msg, "set cases_to %d",i);
 cmd(inter, msg);
 *choice=0;
 goto here_setall;
}



cmd(inter, "destroy .head .f1 .f2 .b");
if( (*choice==1 && res!=0) || *choice==9 || *choice==10)
{

cmd(inter, "set choice $use_seed");
if(*choice==1)
 {
  cmd(inter, "set choice $rnd_seed");
  init_random(*choice);
 }
cmd(inter, "set choice $to_all");
to_all=*choice;
cmd(inter, "set choice $cases_from");
cases_from=*choice;
cmd(inter, "set choice $cases_to");
cases_to=*choice;
cmd(inter, "set choice $update_d");
update_description=*choice;

switch(res)
{
//Equal 
case 1:
     cmd(inter, "set choice $fill");
     fill=*choice;
     cmd(inter, "set choice $step_in");
      
     for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
      {
      if((to_all==1 || (cases_from<=i && cases_to>=i)) && (fill==1 || ((i-cases_from)%(*choice)==0)))
       {
        cv=cur->search_var(NULL, lab);
  			cv->data_loaded='+';
  			if(cv->param==0)
  			  cv->val[lag]=value1;
  			else
  			  cv->val[0]=value1;
       }
		  }
      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
    
      if(to_all==1)
        {sprintf(msg, " All %d instances equal to %g.", i-1, value1);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {
         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d equal to %g.",cd->init, cases_from, cases_to, value1);
         else
           sprintf(msg, "Instances from %d to %d equal to %g.", cases_from, cases_to, value1);  
         change_descr_lab(lab, "", "", "", msg);
        }  
       } 
		  break;

//Range
case 9:
     cmd(inter, "set choice $fill");
     fill=*choice;
     cmd(inter, "set choice $step_in");
     counter=-1;
     for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
      {
      if((to_all==1 || (cases_from<=i && cases_to>=i)) && (((i-cases_from)%(*choice)==0)))
       {
         //here the counter
         counter++;
       }
		  }
     value=(value2-value1)/counter;
     counter=0; 
     for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
      {
      if((to_all==1 || (cases_from<=i && cases_to>=i)) && (fill==1 || ((i-cases_from)%(*choice)==0)))
       {
        cv=cur->search_var(NULL, lab);
  			cv->data_loaded='+';
  			if(cv->param==0)
  			  cv->val[lag]=value1+value*counter;
  			else
  			  cv->val[0]=value1+value*counter;
       }
      if(i>=cases_from && ((i-cases_from+1)% (*choice))==0)
        counter++; 

		  }
      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
      
      if(to_all==1)
        {sprintf(msg, " All %d instances set ranging from %g to %g (i.e. increments of %g).", i-1, value1, value2, value);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {
         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d ranging from %g to %g (i.e. increments of %g).",cd->init, cases_from, cases_to, value1, value2, value);
         else
           sprintf(msg, "Instances from %d to %d ranging from %g to %g (i.e. increments of %g).", cases_from, cases_to, value1, value2, value);  
         change_descr_lab(lab, "", "", "", msg);
        }  
       } 
		  break;

case 2: //increasing
    
     
     cv=r->search_var(NULL, lab);
  	  cv->data_loaded='+';
     cmd(inter, "set choice $fill");
     fill=*choice;
     cmd(inter, "set choice $step_in");
     
      for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
        {
        if((to_all==1 || (cases_from<=i && cases_to>=i)) && (fill==1 || ((i-cases_from)%(*choice)==0)))
        {
         cv=cur->search_var(NULL, lab);
         cv->data_loaded='+';
         if(cv->param==0)
           cv->val[lag]=value1 +step*value2;
         else
           cv->val[0]=value1 +step*value2;
        }
        if(i>=cases_from && ((i-cases_from+1)% (*choice))==0)
          step+=1;
        }
      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
      
      if(to_all==1)
        {sprintf(msg, " All %d instances increasing from %g with steps %g. The value is increased every %d instances.", i-1, value1, value2, *choice);
         change_descr_lab(lab, "", "", "", msg);
        }
      else
        {
         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d increasing from %g with steps %g. The value is increased every %d instances.",cd->init, cases_from, cases_to, value1, value2, *choice);
         else
           sprintf(msg, "Instances from %d to %d increasing from %g with steps %g. The value is increased every %d instances.", cases_from, cases_to, value1, value2, *choice);            
         change_descr_lab(lab, "", "", "", msg);
        }  
        }
        break;
case 4: cv=r->search_var(NULL, lab);
		  for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
        {
      if(to_all==1 || (cases_from<=i && cases_to>=i))
      {
        cv=cur->search_var(NULL, lab);
        cv->data_loaded='+';
        if(cv->param==0)
           cv->val[lag]=value1 +step*value2;
		   	else
           cv->val[0]=value1 +step*value2;
        if(cur->next!=cur->hyper_next(r->label))
          step=-1;
        step+=1;

        }
       } 

      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
      
      if(to_all==1)
        {sprintf(msg, " All %d instances increasing from %g with steps %g re-starting for each group of Objects.", i-1, value1, value2);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {
         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d increasing from %g with steps %g re-starting for each group of Objects.",cd->init, cases_from, cases_to, value1, value2);
         else
           sprintf(msg, "Instances from %d to %d increasing from %g with steps %g re-starting for each group of Objects.", cases_from, cases_to, value1, value2);
           
         change_descr_lab(lab, "", "", "", msg);        
        }  
       }

        break;

case 3: 
     cmd(inter, "set choice $fill");
     fill=*choice;
     cmd(inter, "set choice $step_in");
      
     for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
      {
      if((to_all==1 || (cases_from<=i && cases_to>=i)) && (fill==1 || ((i-cases_from)%(*choice)==0)))
       {
       cv=cur->search_var(NULL, lab);
       cv->data_loaded='+';
       if(cv->param==0)
 			  cv->val[lag]=value1+RND*(value2-value1);
 			else
 			  cv->val[0]=value1+RND*(value2-value1);
       }
      }

      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
      
      if(to_all==1)
        {sprintf(msg, " All %d instances set to random values drawn from a uniform in the range [%g,%g].", i-1, value1, value2);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {

         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d set to random values drawn from a uniform in the range [%g,%g].", cd->init, cases_from, cases_to, value1, value2);
         else
           sprintf(msg, "Instances from %d to %d set to random values drawn from a uniform in the range [%g,%g].", cases_from, cases_to, value1, value2);         
         change_descr_lab(lab, "", "", "", msg);        
        }  
       }


		  break;
case 5: 
      
      
     cmd(inter, "set choice $fill");
     fill=*choice;
     cmd(inter, "set choice $step_in");
      
     for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
      {
      if((to_all==1 || (cases_from<=i && cases_to>=i)) && (fill==1 || ((i-cases_from)%(*choice)==0)))
       {
			cv=cur->search_var(NULL, lab);
         cv->data_loaded='+';
			if(cv->param==0)
			  cv->val[lag]=norm(value1, value2);
			else
			  cv->val[0]=norm(value1, value2);
		  }
      }

      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
      
      if(to_all==1)
        {sprintf(msg, " All %d instances set to random values drawn from a normal with mean=%g and std. deviation=%g.", i-1, value1, value2);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {

         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d set to random values drawn from a normal with mean=%g and std. deviation=%g",cd->init, cases_from, cases_to, value1, value2);
         else
           sprintf(msg, " Instances from %d to %d set to random values drawn from a normal with mean=%g and std. deviation=%g", cases_from, cases_to, value1, value2);         
           change_descr_lab(lab, "", "", "", msg);        
        }  
       }

		  break;

case 6: cv=r->search_var(NULL, lab);
		  cv->data_loaded='+';
        for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label),i++)
        {
       if(to_all==1 || (cases_from<=i && cases_to>=i))
        {
         cv=cur->search_var(NULL, lab);
         cv->data_loaded='+';
         if(step==value2)
          {if(cv->param==0)
            cv->val[lag]=value1;
           else
            cv->val[0]=value1;
           step=0;
          }
         else
           step++;

        }
        }

      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
      
      if(to_all==1)
        {sprintf(msg, " All %d instances set to %g skipping %g instances.", i-1, value1, value2);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {
         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d set to %g skipping %g instances.",cd->init, cases_from, cases_to, value1, value2);
         else
           sprintf(msg, " Instances from %d to %d set to %g skipping %g instances.", cases_from, cases_to, value1, value2);         
         change_descr_lab(lab, "", "", "", msg);        
        }  
       }

        break;


case 7:

cmd(inter, "set oldpath [pwd]");
cmd(inter, "toplevel .a");
cmd(inter, "label .a.l -text \"Insert file name\"");
cmd(inter, "entry .a.e -width 50 -textvariable filename");
*choice=0;
cmd(inter, "button .a.ok -text Ok -command {set choice 1}");
cmd(inter, "button .a.esc -text Escape -command {set choice 2}");
cmd(inter, "button .a.se -text Search -command {set filename [tk_getOpenFile -filetypes {{{\"Text Files\"} {.txt}}}]; raise .a; focus -force .a}");
cmd(inter, "pack .a.l .a.e .a.se .a.ok .a.esc ");
cmd(inter, "set w .a; wm withdraw $w; update idletasks; set x [expr [winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 - [winfo vrootx [winfo parent $w]]]; set y [expr [winfo screenheight $w]/2 - [winfo reqheight $w]/2 - [winfo vrooty [winfo parent $w]]]; wm geom $w +$x+$y; update; wm deiconify $w");
cmd(inter, "raise .a");
while(*choice==0)
  Tcl_DoOneEvent(0);
cmd(inter, "destroy .a");
if(*choice==2)
 break;
l=(char *)Tcl_GetVar(inter, "filename",0);
if(l!=(char *)NULL && strcmp(l, ""))
 { cmd(inter, "cd [file dirname $filename]");
   cmd(inter, "set fn [file tail $filename]");
   l=(char *)Tcl_GetVar(inter, "fn",0);
   f=fopen(l, "r");
   cmd(inter, "cd $oldpath");
   if(f!=NULL)
    {
    fscanf(f, "%s", ch); //the label
    kappa=fscanf(f, "%lf", &value);
    for(i=1,cur=r; cur!=NULL && kappa!=EOF; cur=cur->hyper_next(r->label),i++)
		  {
      if(to_all==1 || (cases_from<=i && cases_to>=i))
      {
      cv=cur->search_var(NULL, lab);
			cv->data_loaded='+';
			if(cv->param==0)
			  cv->val[lag]=value;
			else
			  cv->val[0]=value;
      kappa=fscanf(f, "%lf", &value);
		  }
     }
     if(cur!=NULL || kappa!=EOF)
      plog("Problem loading data. The file may contain a different number of values compared to the Objects to initialize.\n");
      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 
      
      if(to_all==1)
        {sprintf(msg, " All %d instances set with data from file %s.", i-1, l);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {

         if(cd->init!=NULL)
         sprintf(msg, "%s Instances from %d to %d with data from file %s", cd->init, cases_from, cases_to, l);
         else
           sprintf(msg, " Instances from %d to %d with data from file %s", cases_from, cases_to, l);         
         change_descr_lab(lab, "", "", "", msg);        
        }  
       }

    
    }
   }
break;

case 8:
     
     cmd(inter, "set choice $fill");
     fill=*choice;
     cmd(inter, "set choice $step_in");
      
     for(i=1,cur=r, step=0; cur!=NULL; cur=cur->hyper_next(r->label), i++)
      {
      if((to_all==1 || (cases_from<=i && cases_to>=i)) && (fill==1 || ((i-cases_from)%(*choice)==0)))
       {
       cv=cur->search_var(NULL, lab);
       cv->data_loaded='+';
       if(cv->param==0)
 			  cv->val[lag]=rnd_integer(value1,value2);
 			else
 			  cv->val[0]=rnd_integer(value1,value2);
       }
      }

      if( update_description==1)
      {
      cd=search_description(lab);
    
      if(cd==NULL)
      {  cv=r->search_var(NULL, lab);
       if(cv->param==0)
         add_description(lab, "Variable", "(no description available)");
       if(cv->param==1)
         add_description(lab, "Parameter", "(no description available)");  
       if(cv->param==2)
         add_description(lab, "Function", "(no description available)");  
       sprintf(msg, "Warning! description for '%s' not found. New one created.\\n", lab);
       plog(msg);
       cd=search_description(lab);
      } 

      if(to_all==1)
        {sprintf(msg, " All %d instances set to integer random values drawn from a uniform in the range [%g,%g].", i-1, value1, value2);
         change_descr_lab(lab, "", "", "", msg);      
        } 
      else
        {

         if(cd->init!=NULL)
           sprintf(msg, "%s Instances from %d to %d set to integer random values drawn from a uniform in the range [%g,%g].", cd->init, cases_from, cases_to, value1, value2);
         else
           sprintf(msg, "Instances from %d to %d set to integer random values drawn from a uniform in the range [%g,%g].", cases_from, cases_to, value1, value2);         
         change_descr_lab(lab, "", "", "", msg);        
        }  
       }


		  break;


default:printf("\nError in set_all\n");
        exit(1);
}
}
Tcl_UnlinkVar(inter, "value1");
Tcl_UnlinkVar(inter, "value2");
Tcl_UnlinkVar(inter, "res");

}





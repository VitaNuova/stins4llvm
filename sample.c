#include <stdio.h>

int encrypt(int input) {
   int output = input * 2 + 2;
return output;
}

void notify(int output) {
   printf("The output of calc is: %d.\n", output);
}

int calc(int input) {
   int output = input + 2;
   return output;
}

int operation2(int input) {
   int output = encrypt(input);
   return output;
}

int operation1(int input) {
   int output = calc(input);
   notify(output);

return output;
}

void loadUI() {
   printf("Doing some magic here.\n");
}

int main() {
  loadUI();
  int res1 = operation1(2);
  int res2 = operation2(2);
}

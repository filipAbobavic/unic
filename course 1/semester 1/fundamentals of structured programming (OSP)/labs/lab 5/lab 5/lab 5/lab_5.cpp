#include <iostream>
#include <vector>
#include <sstream>
#include <string>
using namespace std;

// функція сортування
void quickSort(vector<int>& a, int left, int right) 
{
    int i = left;
    int j = right;
    int pivot = a[(left + right) / 2]; // візьмем медіану як пріорітетне число

    while (i <= j) 
    {
        //Рухаємось вправо, поки і-тий елемент не більше за опорний
        while (a[i] < pivot) i++;

        //Рухаємось вправо, поки j-тий елемент не меньше за опорний
        while (a[j] > pivot) j--;

        if (i <= j) 
        {
            swap(a[i], a[j]);
            i++;
            j--;
        }
    }
    
    //рекурентно повторюємо, поки не відсортуємо
    if (left < j) quickSort(a, left, j);

    if (i < right) quickSort(a, i, right);
}

int main() {
    vector<int> a;
    int x;
    
    // введення масиву
    cout << "Enter vector elements (example: 1 2 3 4 5): ";
    
    string line;
    getline(cin, line);
    stringstream ss(line);

    while (ss >> x) 
    {
        a.push_back(x);
    }
    
    // сортування з перевіркою
    if (!a.empty()) quickSort(a, 0, a.size() - 1);
    else cout << "Empty vector";
    
    cout << "Sorted vector: ";

    for (int num : a) cout << num << " ";

    return 0;
}
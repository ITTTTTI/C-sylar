#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <map>
using namespace std;

// bool can_achieve_growth(int left,int right,vector<int> a, vector<int> b,int m)
// {
//     sort(a.begin(),a.begin()+(left+right)/2);
//     sort(b.begin(),b.begin()+(left+right)/2);
//     int now_score=0;
//     for(int i=left;i<(left+right)/2;i++)
//     {
//         if(now_score>=m)
//            return true;
//         else
//            now_score+=a[i]*b[i];
//     }
//     return false;
// }

// int main() {
//     int n;
//     long long m;
//     map<int,int> val_index;
    
//     cin >> n >> m;
    
//     vector<int> a(n);
//     vector<int> b(n);

//     for (int i = 0; i < n; ++i) {
//         cin >> a[i];
//     }

//     for (int i = 0; i < n; ++i) {
//         cin >> b[i];
//     }
//     int left=0;
//     int right=n-1;
//     bool can_achieve = false;
//     while(left<right){
//         can_achieve=can_achieve_growth(left,right,a,b,m);
//         {
//             if(can_achieve)
//             {
//                 right=(left+right)/2;
//             }
//             else
//             {
//                 left=(left+right)/2;
//             }

//         }
//     }

//     if(can_achieve)
//     {
//         return 
//     }



    

//     return 0;
// }

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>

using namespace std;

// 检查字符频率能否划分为指定周期长度的伪周期串
bool canDivide(const vector<int>& freq, int periodLength) {
    vector<int> periodFreq(10, 0);
    for (int i = 0; i < 10; ++i) {
        if (freq[i] % periodLength != 0) {
            return false;
        }
        periodFreq[i] = freq[i] / periodLength;
    }
    
    // 检查是否所有字符的频率都符合周期要求
    int totalFrequency = 0;
    for (int i = 0; i < 10; ++i) {
        totalFrequency += periodFreq[i];
    }
    
    return totalFrequency * periodLength == accumulate(freq.begin(), freq.end(), 0);
}

int main() {
    int n;
    cin >> n;
    
    string T;
    cin >> T;
    
    // 统计字符频率
    vector<int> freq(10, 0);
    for (char c : T) {
        freq[c - '0']++;
    }
    
    int maxK = 1;
    for (int len = 1; len <= n; ++len) {
        if (n % len == 0) {
            if (canDivide(freq, len)) {
                maxK = n / len;
            }
        }
    }
    
    cout << maxK << endl;
    return 0;
}

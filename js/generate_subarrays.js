function generate_subarrays(arr) {
    let sub_arrs = [];
    for (let i = 0; i < arr.length; i++) {
        let sub_arr = [];
        for (let j = i; j < arr.length; j++) {
            sub_arr.push(arr[j]);
            sub_arrs.push([]);
            for (let k = 0; k < sub_arr.length; k++) {
                sub_arrs[sub_arrs.length-1][k] = sub_arr[k];
            }
        }
    }

    return sub_arrs;
}
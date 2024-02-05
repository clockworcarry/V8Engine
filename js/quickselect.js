function quickselect(arr, left, right, k) {
    if (left < right) {
        let part_idx = partition(arr, left, right);

        if (part_idx == k) {
            return arr[part_idx];
        }
        else if (part_idx < k) {
            return quickselect(arr, part_idx+1, right, k);
        }
        else {
            return quickselect(arr, left, part_idx-1, k);
        }

        return -1;
    }
}

function partition(arr, left, right) {
    let random_idx = Math.floor(Math.random() * (right - left + 1) + left);

    let tmp = arr[right];
    arr[right] = arr[random_idx];
    arr[random_idx] = tmp;

    let pivot = arr[right];

    let i = left - 1;
    for (let j = left; j < right; j++) {
        if (arr[j] <= pivot) {
            i += 1;
            let tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
    }

    tmp = arr[i+1];
    arr[i+1] = arr[right];
    arr[right] = tmp;

    return i+1;
}
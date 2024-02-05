function two_sum(nums, target) {
    let numIndices = {};
    for (let i = 0; i < nums.length; i++) {
        let complement = target - nums[i];
        if (numIndices[complement] !== undefined) {
            return [numIndices[complement], i];
        }
        numIndices[nums[i]] = i;
    }

    return [];
}
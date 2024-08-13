
is draw tree valid? {

	how do you figure out that the validation algorithm should be just like this {
		do not know yet
	}

	draw_tree_array is the array to validate

	what makes it not valid {
		if a draw order goes back to a draw order than invokes iteself you get a never ending loop
		if there are other types than WINDOW RENDERTEXTURE or DRAWORDER or the id is not 0xFFFFFFFF in the draw_tree_array
		if a DRAWORDER doesnt follow a TARGET (WINDWO or RENDERTETURE)
	}

	the algorithm is {
		go through each branch to see if any branch goes back to itself {
			you check this by marking the branch you are going through, one by one, as visited and when bachpropagering to the next brach you mark it as not visited;
			if a branch is valid... but first:
			what makes a branch valid? {
				{
					if you have reached the end you can mark the branch valid all the way back to where an other branch splits off which is not visited nor is valid.
					if this other branch is valid you can continue backwards on the branch
					if this other branch is visited and valid there is design flaw
					if this other branch is visited and not valid the draw tree is not valid
				}
				{
					if the brach meets a splitting point where all options are valid then itself is valid as well and the backpropagation can begin.

				}

			}
			if a branch is valid
		}
		what is a branch? {
			a branch is all draw orders contained in one draw order
			what is a draw order? {
				a draw order is a list of items in which they should be draw//
				its not for any specific target, because it is the targets which stores the draw order which they want to be drawn onto them//

			}
		}
	}
}

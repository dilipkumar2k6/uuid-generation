package lib

type IdGenerator interface {
	NextIdString() string
}

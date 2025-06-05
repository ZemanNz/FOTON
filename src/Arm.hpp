struct Arm
{

    void SmallerFront(){
        man.stupidServo(3).setPosition(2);
    }
    void SmallerBack(){
        man.stupidServo(3).setPosition(-1.8);
    }
    void SmallerUp(){
        man.stupidServo(3).setPosition(0);
    }
    void BiggerFront(){
        man.stupidServo(2).setPosition(-1.9);
    }
    void BiggerBack(){
        man.stupidServo(2).setPosition(2);
    }
    void BiggerUp(){
        man.stupidServo(2).setPosition(0);
    }

};